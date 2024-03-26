 ![](Wynn.PNG)
==============

specials\_repeat
================

This small version of Joy is claimed to be almost free of bugs.

Even so, there are some surprises.

In the library there is a definition of contains that contains the construct
\[ = or\]. The space character between \[ and = cannot be removed.

The reason for that is the specials\_repeat that contains = and >. These
characters allow == and <> to be recognized as tokens, but they are generic.
That means that \[= will also be recognized as a token.

Fortunately, this leads to an error message: "bad order in library" and the
line that contains contains is repeated.

It might be possible to write some extra code that prevents the message, but
that does not seem worth the effort.

The reason for maintaining this version of Joy is that it is an order of
magnitude smaller than the full version.

And it has some features that are not present in full Joy, such as scantime
expression evaluation.
