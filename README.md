# Switch/Case strings using constexpr long hash

## The Problem

* To avoid questionable code like this:

```c++
} else if (key == "poney") {
} else if (key == "elephant") {
} else if (key == "dog") {
} else if (key == "kitten") {
```

ie.: *cumbersome, and not quite good in term of performances*

* We basically want to do that in `C++`:

```c++
switch(argv[i]) {
case "poney":
  break;
case "elephant":
  break;
case "dog":
  break;
case "kitten":
  break;
}
```

## The Solution

We use a `constexpr` metaprogrammed long hash, with fancy custom string prefixes, allowing to do *nearly* what we want:

```c++
switch(fnv1a128::hash(argv[i])) {
case "poney"_fnv1a128:
  break;
case "elephant"_fnv1a128:
  break;
case "dog"_fnv1a128:
  break;
case "kitten"_fnv1a128:
  break;
}
```

👉 See the [`switch_fnv1a.h`](switch_fnv1a.h) source code and the [`main.cpp`](main.cpp) sample.

## Discussion

### Is This Good Enough ?

#### Simplicity

Nearly what we want, only need a call to `fnv1a128::hash` in the `switch`, and `_fnv1a128` string prefix in `case`.

#### Collisions

Using a rather known *128-bit* hashing (Fnv1-a) *should* limit the risks of accidental collisions (odds are `1/2^64` if the hash is truly distributed). FNV hashes are typically [designed to be fast while maintaining a low collision rate](https://tools.ietf.org/html/draft-eastlake-fnv-16).

Collision rate of `1/2^64` is considered low enough not to worry about them. Risks of being killed by a meteor each year (> `1/2^27`) is several orders of magnitude more worrying, actually ([The Economist](https://www.cnet.com/news/odds-of-dying-from-an-asteroid-strike-1-in-74817414/) and [W. Casey, *Cybersecurity and Applied Mathematics*](https://www.sciencedirect.com/book/9780128044520/cybersecurity-and-applied-mathematics)).

#### Attacks

Question remains on *non-accidental* collisions (ie. *attacks*). One possible solution would be to use a stronger hash (ie. cryptographic hash) such as SHA256 (~~XOR-folded~~ [truncated](https://csrc.nist.gov/csrc/media/events/first-cryptographic-hash-workshop/documents/kelsey_truncation.pdf) to 128-bits), such as [this one](https://github.com/aguinet/sha256_literal) or [this one](https://github.com/elbeno/constexpr/), but this would make the initial string computation much slower, not mentioning the build time.

As [suggested](https://tools.ietf.org/html/draft-eastlake-fnv-16#section-2.2), mitigating possible attacks could involve a initial salt, such as `fnv1a128::hash(__FILE__)` or `fnv1a128::hash(__DATE__)` (with non-reproducible build), but this would not allow to mitigate attacks when compiled code itself is known.

#### Performances

* All hash string literal are computed at **build-time**
* Hash is computed once at runtime (for the input to be compared) with a reasonable cost (ie. one 128-bit multiply and one 8-bit xor per character)
* The `switch/case` construct is rather well-optimized (using binary search-like dispatch, ie. `O(log(N))`)
* 128-bit `__uint128_t` are rather well-handled for this usage (thanks to AVX extensions)

Three `std::string::operator==` calls are typically as slow as a complete `switch/case` set containing 1,000 different cases.

*All these results need to be taken with a grain of salt, being based on highly volatile micro-benchmarks, you have been warned.*

#### Benchmarks (micro-benchmarks only)

Quick benchmarks with output emitted (redirected to `/dev/null`)

* Matching 100,000 times 100 different strings with **10 cases**, printing them to stdout: **1.5x** faster
* Matching 100,000 times 100 different strings with **100 cases**, printing them to stdout: **3.9x** faster
* Matching 100,000 times 100 different strings with **1000 cases**, printing them to stdout: **120x** faster

Quick benchmarks with no output emitted (pure computation)

* Matching 100,000 times 100 different strings with **10 cases**: **2.3x** faster
* Matching 100,000 times 100 different strings with **100 cases**: **9x** faster
* Matching 100,000 times 100 different strings with **1000 cases**: **400x** faster

Further tests are needed to assess the impact on performances on real-world code (such as configuration parsing code typically).

#### Unit Tests

The wonders of meta-programming allows you to actually integrate unit tests in the code itself:

```c++
// Static unit tests: <https://fnvhash.github.io/fnv-calculator-online/>
static_assert("hello"_fnv1a32 == 0x4f9f2cab);
static_assert("hello"_fnv1a64 == 0xa430d84680aabd0b);
static_assert("hello"_fnv1a128 == Pack128(0xe3e1efd54283d94f, 0x7081314b599d31b3));
```

### Disasembled Example

As an example, let's see how the compiler dispatches four `case`:

```c++
const char* dispatch(const __uint128_t match)
{
    switch (match) {
    case "poney"_fnv1a128:
        return "I want one, too!";
    case "elephant"_fnv1a128:
        return "Not in my apartment please!";
    case "dog"_fnv1a128:
        return "Good puppy!";
    case "kitten"_fnv1a128:
        return "Aawwwwwwww!";
    default:
        return "Don't know this animal!";
    }
}
```

Even for four cases (and a default), the compiler optimized already the tests by with binary search: the "dog" and poney cases blocks are split from the "kitten" and "elephant" ones.

The first test is done through two regular 64-bit comparisons (and immediate values), the remaining ones are using AVX extensions (comparisons with PC-relative indexed address).

```asm
movabs $0x6efcb17ab10f2a3d,%rax # ("kitten"_fnv1a128 - 1)&FFFFFFFF
cmp    %rdi,%rax
movabs $0xfcd05704e13c64bf,%rax # ("kitten"_fnv1a128 - 1)>> 64
movq   %rdi,%xmm0
movq   %rsi,%xmm1
punpcklqdq %xmm1,%xmm0
sbb    %rsi,%rax
jl     0x400bba <test_kitten>

movdqa 0x17bce(%rip),%xmm1      # "dog"_fnv1a128 (__uint128_t)
pcmpeqb %xmm0,%xmm1
pmovmskb %xmm1,%eax
cmp    $0xffff,%eax
je     0x400bf0 <puppy>

pcmpeqb 0x17bc7(%rip),%xmm0     # "poney"_fnv1a128 (__uint128_t)
pmovmskb %xmm0,%eax
cmp    $0xffff,%eax
jne    0x400bea <unknown>

mov    $0x41c6a0,%eax           # "I want one, too!" (const char*)
retq

test_kitten:
movdqa 0x17b7e(%rip),%xmm1      # "kitten"_fnv1a128 (__uint128_t)
pcmpeqb %xmm0,%xmm1
pmovmskb %xmm1,%eax
cmp    $0xffff,%eax
je     0x400bf6 <kitten>

pcmpeqb 0x17b77(%rip),%xmm0     # "elephant"_fnv1a128 (__uint128_t)
pmovmskb %xmm0,%eax
cmp    $0xffff,%eax

jne    0x400bea <unknown>
mov    $0x41c6b1,%eax           # "Not in my apartment please!" (const char*)
retq

unknown:
mov    $0x41c6e6,%eax           # "Don't know this animal!" (const char*)
retq

puppy:
mov    $0x41c6ce,%eax           # "Good puppy!" (const char*)
retq

kitten:
mov    $0x41c6da,%eax           # "Aawwwwwwww! (const char*)
retq
```

The initial hash computation itself is rather well-optimized, as the 128-bit prime number used (0x1000000000000000000013b) is well-fit for split 64-bit `mul`.

```asm
test   %rsi,%rsi
je     0x400de9 <empty_string>

movabs $0x6c62272e07bb0142,%rcx
movabs $0x62b821756295c58d,%rax
lea    -0x1(%rsi),%rdx
mov    %esi,%r9d
and    $0x3,%r9d
cmp    $0x3,%rdx
jae    0x400dfe <label2>
xor    %edx,%edx
xor    %r10d,%r10d
test   %r9,%r9
jne    0x400ea9 <label4>

label_ret:
retq

empty_string:
movabs $0x6c62272e07bb0142,%rdx
movabs $0x62b821756295c58d,%rax
retq

label2:
push   %rbx
sub    %r9,%rsi
xor    %r10d,%r10d
mov    $0x13b,%r11d
nopl   0x0(%rax,%rax,1)

label3:
movzbl (%rdi,%r10,1),%r8d
xor    %rax,%r8
mov    %r8,%rax
mul    %r11
shl    $0x18,%r8
add    %rdx,%r8
imul   $0x13b,%rcx,%rbx
add    %r8,%rbx
movzbl 0x1(%rdi,%r10,1),%ecx
xor    %rax,%rcx
mov    %rcx,%rax
mul    %r11
shl    $0x18,%rcx
add    %rdx,%rcx
imul   $0x13b,%rbx,%rbx
add    %rcx,%rbx
movzbl 0x2(%rdi,%r10,1),%ecx
xor    %rax,%rcx
mov    %rcx,%rax
mul    %r11
shl    $0x18,%rcx
add    %rdx,%rcx
imul   $0x13b,%rbx,%rdx
add    %rcx,%rdx
movzbl 0x3(%rdi,%r10,1),%ecx
xor    %rax,%rcx
imul   $0x13b,%rdx,%rbx
mov    %rcx,%rax
mul    %r11
shl    $0x18,%rcx
add    %rdx,%rcx
add    %rbx,%rcx
add    $0x4,%r10
cmp    %r10,%rsi
jne    0x400e10 <label3>
mov    %rcx,%rdx
pop    %rbx
test   %r9,%r9
je     0x400de8 <label_ret>

label4:
add    %r10,%rdi
neg    %r9
mov    $0x13b,%r8d
data16 nopw %cs:0x0(%rax,%rax,1)
label5:
movzbl (%rdi),%esi
xor    %rax,%rsi
mov    %rsi,%rax
mul    %r8
shl    $0x18,%rsi
add    %rdx,%rsi
imul   $0x13b,%rcx,%rcx
add    %rsi,%rcx
add    $0x1,%rdi
add    $0x1,%r9
jne    0x400ec0 <label5>
mov    %rcx,%rdx
retq
```

## Thanks

Thanks to [Algolia](https://www.algolia.com/) for giving me the opportunity to play with those kind of stuff during my work!
