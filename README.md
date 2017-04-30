# Lambda
A simple lambda calculus interpreter based on the book ["An Introduction to Functional Programming Through Lambda Calculus"](https://www.amazon.com/Introduction-Functional-Programming-Calculus-Mathematics/dp/0486478831) by Greg Michaelson

The source consists of a simple recursive-descent parser for parsing lambda expressions, and a reduction engine

Example:
(λx.x λx.x)
---
Eval "(λx.x λx.x)"
... => λx.x
