# Two Sum

**Difficulty:** Easy · **Topics:** Array, Hash Map · **Companies:** [Meta](../../../company-wise/meta.md), [Google](../../../company-wise/google.md), [Amazon](../../../company-wise/amazon.md)

## Problem Statement
Given an array of integers `nums` and an integer `target`, return the indices of the two numbers that add up to `target`. Exactly one solution exists, and the same element may not be used twice.

## Approach
**Brute force:** check every pair.

**Optimal (one-pass hash map):** for each `x`, look up `target - x` in a map of values seen so far. If found, return both indices; otherwise store `x`.

## Complexity
| Approach | Time | Space |
|---|---|---|
| Brute force | O(n²) | O(1) |
| Hash map | O(n) | O(n) |

## Edge Cases
- Duplicates (`[3, 3]`, target `6`)
- Negative numbers and zero

## Solutions
- [Python](solution.py)
- [C++](solution.cpp)
