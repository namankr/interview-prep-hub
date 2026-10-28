## 1. GenAI & Large Language Models (LLMs)

**Q: What is the difference between Top-K and Top-P sampling in LLMs, and when would you use each?**
- Top-K limits the model to choosing from the K highest-probability words. Top-P (nucleus sampling) chooses from a dynamic pool of words whose cumulative probability reaches threshold P. Use Top-K for high structural consistency and Top-P for diverse, creative responses.

**Q: What does the Temperature parameter do in an LLM?**
- It scales the logits before the softmax layer to control response randomness. A lower temperature (e.g., 0.1–0.3) makes the model deterministic and focused, while a higher temperature (e.g., 0.7–1.0) increases creativity and randomness.

**Q: How can you force an LLM to reliably return outputs in a structured schema like JSON or CSV?**
- By using structured outputs via JSON schema enforcement (supported natively by APIs like OpenAI/Azure), strict system prompt styling, or utilizing open-source steering libraries like Guidance or Instructor.
- See also: [Abstract Syntax Tree (AST) details](#abstract-syntax-tree-ast) and [How to index a codebase](#how-to-index-the-codebase) — structured/schema-constrained parsing of code follows the same principle as enforcing JSON/CSV schemas on LLM output.

## 2. RAG & Enterprise System Design

**Q1: How would you design a RAG system for a massive, multi-directory corporate repository without blowing past context windows?**
(Also phrased as: Your AI coding assistant must work across a codebase with millions of lines of code. The entire repo cannot fit in the model's context window. How would you help the AI understand and navigate the codebase effectively?)

- Instead of feeding the whole codebase, index it hierarchically. Chunk the repository by structure (file, class, function), extract an Abstract Syntax Tree (AST) to track structural dependencies, and use a vector database to fetch only the explicitly relevant chunks per query.

1. **Don't feed the code — index it.** Chunk the repo (by file, class, function) and store embeddings in a vector DB. Retrieve only the most relevant snippets per query instead of dumping the whole tree.
2. **Build a code map, not just chunks.** Parse the AST to capture structure — imports, call graphs, dependencies, symbol definitions. The AI navigates relationships, not raw text.
3. **Hierarchical summarization.** Summarize at every level: function → file → module → repo. The model reads a cheap high-level map first, then drills down only where needed.
4. **Agentic, tool-based retrieval.** Give the AI tools — `grep`, `find_definition`, `list_references`, `read_file` — and let it search on demand.
5. **Keep context fresh.** Re-index on commits, invalidate stale embeddings, and cache hot paths so retrieval always reflects the latest code.

### Abstract Syntax Tree (AST)

An Abstract Syntax Tree (AST) is a hierarchical, tree-like representation of the structural components of source code. Unlike raw text, an AST strips away superficial syntax details (like semicolons, brackets, and whitespace) and focuses purely on the structural logic of the program — such as variable declarations, loops, functions, and conditional branches.

In modern software engineering, ASTs are the backbone of advanced code analysis, linting, compilation, and LLM-driven codebase intelligence (like the hierarchical mapping and agentic retrieval described above).

#### How to Prepare (Generate) an AST

Preparing an AST involves parsing your source code into a structured tree object. The exact implementation depends on the language you are parsing, but the workflow generally follows these architectural steps:

```
Source Code ──> [ 1. Lexical Analysis (Tokens) ] ──> [ 2. Syntactic Analysis (Parser) ] ──> AST Object
```

**1. Choose the right parser for your stack**

You rarely need to write a parser from scratch. Instead, leverage battle-tested parsing libraries based on the language you are processing:
- Multi-language support: Tree-sitter (highly recommended for LLM tools as it supports incremental parsing on file saves), Babel (JavaScript/TypeScript), or ANTLR.
- Python native: the built-in `ast` module.
- Rust native: `syn` (for Rust code parsing).

**2. The implementation workflow**

To prepare an AST for use in code intelligence tools (like indexing or context retrieval), you follow a three-step processing pipeline. Here is a practical example using Python's native AST engine:

*Step A: Parse the raw source code*

Load your code file and compile it into the core tree structure.

```python
import ast

# Sample code snippet to parse
source_code = """
def calculate_area(radius):
    pi = 3.14159
    return pi * (radius ** 2)
"""

# Generate the AST
tree = ast.parse(source_code)
```

*Step B: Traverse the tree (the Node Visitor pattern)*

An AST consists of various node types (e.g., `FunctionDef`, `Assign`, `Return`). To map your codebase structure, you write a "Visitor" class that triggers specific actions when it encounters these nodes.

```python
class CodeMapGenerator(ast.NodeVisitor):
    def visit_FunctionDef(self, node):
        print(f"Found Function Definition: '{node.name}' at line {node.lineno}")
        # Continue traversing inside the function to find assignments/returns
        self.generic_visit(node)

    def visit_Assign(self, node):
        # Capture variable assignments for symbol tracking
        for target in node.targets:
            if isinstance(target, ast.Name):
                print(f"  Variable declared: '{target.id}'")
        self.generic_visit(node)

# Initialize and run the mapper
mapper = CodeMapGenerator()
mapper.visit(tree)
```

*Step C: Build the graph meta-data*

To turn the raw AST into a usable code map for an AI agent, extract the structural components during traversal and format them into a relational schema:
- Identify symbols: extract all function names, class definitions, and imported modules.
- Resolve dependencies: map which files import which modules.
- Trace the call graph: look for `ast.Call` nodes inside functions to map out which functions invoke other functions.

### How to Index the Codebase

To index a codebase efficiently for AI-driven intelligence, you need to combine semantic vector search with an AST-derived structural code map. This dual approach ensures your AI agent understands both the natural language intent of a query and the strict logical relationships of the system.

Here is a production-grade pipeline to index a codebase based on the architecture outlined above.

**Step 1: Chunk the repository (hierarchical strategy)**

Do not split code arbitrarily by character count. Instead, use an AST parser (like Tree-sitter) to extract clean, syntactically whole chunks.
1. Extract chunks: split files by their logical boundaries — classes, methods/functions, and the remaining standalone module-level code.
2. Inject contextual headers: a disconnected code snippet loses its meaning. Before generating embeddings, prepend structural metadata to the top of each chunk.

```python
# --- Context Injection Example ---
# File: src/auth/jwt.py
# Class: JWTManager
# Function: verify_token(token: str) -> dict
# Dependencies: jwt, datetime, src.errors.TokenExpired

def verify_token(token):
    # Actual implementation code goes here...
```

**Step 2: Generate dual embeddings**

Code queries usually fall into two categories: "How do I authenticate a user?" (semantic) and "Where is `verify_token` defined?" (exact/symbolic). To handle both, index your chunks twice:
- Dense vectors: use models optimized for code search (e.g., `text-embedding-3-large`, `voyage-code-3`, or local `stella_en_400M_v5`) to capture semantic intent.
- Sparse vectors (BM25): index the raw tokens, function signatures, and variable names to ensure flawless exact-match lookups for specific variable names or system errors.

**Step 3: Build the relational code map (graph DB / SQL)**

While chunks go into a vector DB, the structure goes into a graph database (like Neo4j) or a relational DB with recursive querying (like PostgreSQL). As you parse the AST of each file, extract and write these entities and relationships:
- Nodes: File, Module, Class, Function, Variable.
- Edges (relationships):
  - File ──[CONTAINS]──> Class
  - Class ──[HAS_METHOD]──> Function
  - Function A ──[CALLS]──> Function B (call graph)
  - File A ──[IMPORTS]──> File B (dependency graph)

**Step 4: Compute hierarchical summaries**

To prevent an LLM from reading thousands of chunks to understand a repository, build a multi-tiered hierarchy. Generate summaries from the bottom up:
1. Level 1 (leaves): read the raw function code → LLM generates a 1-sentence summary of the function.
2. Level 2 (files): combine all function summaries of a file → LLM generates a high-level summary of the file's responsibility.
3. Level 3 (modules/directories): combine file summaries → generate a module summary.

Where to store it: attach these summaries directly as metadata fields on your code map nodes or vector metadata. When an AI agent starts a task, it reads the module and file summaries first to narrow down the target path.

**Step 5: Establish the live invalidation loop**

Code changes constantly. Your index must stay accurate without full, expensive re-indexing runs on every commit.

```
git push ──> Webhook ──> Identify Changed Files ──> Purge Old Nodes/Vectors ──> Re-Parse & Index
```

1. Git hook integration: set up a CI/CD pipeline or GitHub webhook triggered on every commit/merge to the main branch.
2. Determine the diff: use `git diff --name-only HEAD~1` to isolate exactly which files were modified, added, or deleted.
3. Targeted purge & re-index:
   - Deleted files: drop all vector IDs and code map nodes associated with that file path.
   - Modified files: delete the old vectors and nodes for that specific file, run the AST parser over the new code, generate fresh chunks/embeddings, and write them back.
4. Cache hot paths: maintain an in-memory cache (like Redis) of frequently accessed structures (e.g., the core API router map or database connection utilities) so the agent can retrieve them instantly without querying the database.

**Q2: How can we handle large chunks in a RAG pipeline?**
- Implement a hierarchical Parent-Child Chunking strategy combined with a Late Chunking / Re-ranking loop.

```
[Document] ──► Split into Large "Parent" Chunks (e.g., 2000 tokens)
                  │
                  └──► Sub-divided into Small "Child" Chunks (e.g., 200 tokens)
                        │
                        ├──► Embed & index only Child Chunks in Vector DB
                        │
                        └──► [Query Match] ──► Retrieve Child, but pass Parent to LLM
```

- **Parent-child relationships:** segment documents into large structural blocks (parents, ~2000 tokens) to preserve overall context, then subdivide them into small sentences or paragraphs (children, ~200 tokens). Embed and search against the child chunks for precise vector matching, but return the broader parent chunk to the LLM to provide rich surrounding context.
- **Late chunking:** generate token embeddings across the entire document sequence before splitting them into chunks. This ensures that boundary tokens within smaller chunks still retain global contextual information from the larger document.
- **Cross-encoder re-ranking:** retrieve a wide pool of smaller chunks (e.g., Top-50 via bi-encoder cosine similarity) and pass them through a computationally heavier cross-encoder (like Cohere Rerank or BGE-Reranker). This filters out the irrelevant fragments, distilling the context window down to the absolute highest-signal data.

**Q: What metrics do you use to evaluate the retrieval stage of a RAG pipeline?**
- Recall@K (how many of the relevant documents were successfully fetched in the top K results) and Precision@K (how many of the fetched top K documents were actually relevant to the query).

## 3. Core Machine Learning & Foundational Math

Traditional ML infrastructure questions ensure you understand what happens beneath the libraries.

**Q: Explain the mechanics of Gradient Descent and Stochastic Gradient Descent (SGD).**
- Gradient Descent computes the gradient of the cost function relative to the entire dataset to update weights. SGD updates the weights iteratively using only a single random training sample (or a mini-batch) at a time, making it faster and able to escape local minima.

**Q: How does K-Means++ improve upon the standard K-Means clustering algorithm?**
- Standard K-Means picks initial cluster centers completely at random, which can lead to poor convergence. K-Means++ selects the first center uniformly at random, but picks subsequent centers based on a probability distribution proportional to their squared distance from the closest existing center, ensuring smarter initialization.

**Q: What is the difference between Precision and Recall, and how do they relate to the Confusion Matrix?**
- Precision ($\frac{TP}{TP+FP}$) measures accuracy among predicted positives. Recall ($\frac{TP}{TP+FN}$) measures the ability to find all actual positives. They are calculated using true positives (TP), false positives (FP), and false negatives (FN) mapped from the matrix.

## 4. Agentic AI & Optimization

These focus on the operational and economic challenges of putting modern AI systems into enterprise production.

**Q: How do you optimize an Agentic AI system to keep operational costs and latency low?**
- Use model routing (sending basic classification requests to cheaper, smaller models and reserving complex reasoning tasks for frontier models), enforce strict token retrieval limits, optimize caching layers for frequent user paths, and cut agent execution loops short with hard step caps.

**Q: When should a project team explicitly avoid using AI and opt for traditional software instead?**
- Avoid AI if a simple rule-based solution is sufficient, as deterministic software is faster, cheaper, and more maintainable. Also, skip AI when absolute mathematical precision is paramount (e.g., core tax calculations) or when the deployment/maintenance cost outweighs the expected business value.