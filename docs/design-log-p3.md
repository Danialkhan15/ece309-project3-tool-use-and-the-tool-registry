# Project 3 Design Log

**Author:** Danial Khan

**Repository:** https://github.com/Danialkhan15/ece309-project3-tool-use-and-the-tool-registry

**P2 implementation:** I used my Project 2 code from
[the P2 repository](https://github.com/Danialkhan15/ece309-project2-starter),
commit `fc26d7ce3cd86d2c8450af449ec6cd94c90f48a3`, with
`USE_P2_REFERENCE=OFF`. I copied the Conversation and SentinelScanner headers
and source files, keeping P3's supplied `message.h`.

**Contributions and tools:** I was responsible for the project design and most
of the implementation. OpenAI Codex assisted with debugging, test development,
and revisions to code readability and this log. I used CMake and Clang to build,
CTest to run tests, and AddressSanitizer and UndefinedBehaviorSanitizer to check
for runtime errors. Git and GitHub CLI handled version control and uploads.

## 1. Registry implementation

I used separate chaining with a `vector` of `list<Entry>` buckets. Each entry
holds the tool's name and a `unique_ptr<Tool>`. A lookup hashes the name, selects
one bucket, and searches that list. Chaining handles collisions by placing
another entry in the same list. Linear probing would avoid list allocations,
but would need more empty slots to keep searches short.

The hash is 64-bit FNV-1a. It starts at `14695981039346656037`, XORs each unsigned
byte into the hash, and multiplies by `1099511628211`. Unsigned overflow wraps
the result. This uses every character and accounts for character order.
`hash()` returns the full value; `% bucket_count()` gives the bucket index.

The table starts with 16 buckets and allows a maximum load factor of 1.0.
Chaining can hold multiple entries per bucket, but this limit keeps the average
chain length small. Before an insertion would exceed the limit, `add()` doubles
the bucket count. `rehash()` recomputes each entry's index and splices its list
node into the new bucket. Null tools and duplicates are rejected before resizing.

For `tool_0` through `tool_9999`, the results were:

| Measurement | FNV-1a | Weak hash: `s[0] + s.size()` |
|---|---:|---:|
| Final bucket count | 16,384 | 16,384 |
| Load factor | 0.6103515625 | 0.6103515625 |
| Longest chain | 6 | 9,000 |

The weak-hash results come from counting bucket assignments without building a
second registry. All 9,000 four-digit suffixes share the same length and first
character, so they land in one bucket. FNV-1a also uses the suffix digits.

Resizing takes O(n), but doubling provides room for about n more insertions.
The entries moved across resizes form a geometric series, so total resize work
grows linearly with the number of insertions. With short expected chains and
bounded name lengths, `add()` is amortized O(1).

## 2. Ownership and pointer stability

The registry owns each tool through one `unique_ptr`. Splicing a list node
changes its bucket without moving the tool object. A pointer from `find()` stays
valid through later insertions and resizes, until the registry is destroyed.
Rejecting a duplicate destroys the incoming tool and keeps the original. The
tests save addresses across resizes and count destructor calls to check both
behaviors.

## 3. Calculator implementation and trace

The evaluator uses `values` and `ops` vectors as stacks, with the top at the
back. For `2 * (3 + 4) - 5`, reading `2` gives values `[2]`; reading `*` gives
operators `[*]`. The opening parenthesis changes the operators to `[*, (]`.

Reading `3`, `+`, and `4` leaves values `[2, 3, 4]` and operators `[*, (, +]`.
At `)`, the evaluator applies `3 + 4` and removes `(`, leaving values `[2, 7]`
and operators `[*]`.

When `-` arrives, multiplication runs first because it has higher precedence.
Values become `[14]`, and operators become `[-]`. Reading `5` gives `[14, 5]`.
The final subtraction leaves `[9]`. Equal-precedence operators also run before
the incoming operator, making subtraction and division work left to right.
The code uses loops throughout and checks stack sizes before accessing operands.

## 4. Workspace paths and error handling

`resolve()` rejects empty paths, NUL bytes, and rooted paths. It normalizes the
remaining text and rejects a leading `..`; `.` resolves to the workspace root.
It does no filesystem checks. The provided symlink check runs separately before
file access.

A forbidden path or division by zero is an input failure the model can recover
from. Returning `false` with an explanation lets the tool report the error and
continue the conversation. Programmer errors, such as using the wrong JSON
accessor, indicate a code defect. The dispatcher catches exceptions as a final
safeguard.

## 5. Possible improvement

I would give tools an immutable registration name. Currently, an entry stores
a name separately from the tool, so changing the tool's reported name could
make them disagree. An immutable name would make that relationship explicit.
The implementation retains the required public interface.
