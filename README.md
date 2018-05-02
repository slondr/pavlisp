# Pavlisp
A small-scale lisp interpreter.
Written by Andrew Soutar and Eric S. Londres, based on previous work by Jonathon Pavlik.

## Copying
Essentially, this software is fully released into the public domain wherever applicable. For details on copying, see `LICENSE`.

## Compiling
For details on compiling, see `Makefile`. This software is developed using GCC version 7 -- it may or may not compile with Clang or older versions of GCC.

## Functionality
For extended details on implementation, see `pavlisp.c`.
Here is a brief overview on functions currently implemented:
* `quote` and `'` do what you expect - return the name of whatever symbol you give it, rather than evaluating it. They are more or less synonymous.
* `cons` takes one or two arguments and returns a pair containing both of them. You can give it more than two arguments, but it will ignore them. `(cons 'symbol)` is equivalent to `'(symbol)`.
* `car` takes a list and returns the first element of the list. If the given argument is not a list, it returns `nil`. Extra arguments are ignored.
* `cdr` takes a list and returns every element except the first. If the list contains zero or one elements, or is not a list, it returns `nil`. Otherwise, it will always return a list.
* `let` allows you to define new symbols. It has similar syntax to Common Lisp, taking first a list of lists of variable definitions (such as `(let ((x 'hello) (y 'world)))` and then an arbitrary amount of other statements in which those symbols can be used. Keep in mind when using `let` that Pavlisp does not support any data types other than lists.

Aside from this core functionality, Pavlisp also supports lambda functions and macros using Common Lisp syntax. 
Also note that attempting to evaluate any function or symbol that does not exist will simply return `nil`.

## Documentation
For details on details, see `README.md`.
