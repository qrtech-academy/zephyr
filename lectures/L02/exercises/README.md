# L02 Exercises - Your Workspace

Write your solutions to the exercises in [Appendix B](../appendix/b_exercises.md) here, one
application per exercise set, with the file names below. That is all it takes for `make test` to
find them:

```text
lectures/L02/exercises/
    exercise1/                    Exercise Set 1: the board's LED
        CMakeLists.txt
        prj.conf
        app.overlay
        src/main.c
    exercise2/                    Exercise Set 2: a button of your own
        CMakeLists.txt
        prj.conf
        app.overlay
        src/main.c
    exercise3/                    Exercise Set 3: a binding of your own
        CMakeLists.txt
        prj.conf
        app.overlay
        dts/bindings/qacademy,alarm.yaml
        dts/bindings/vendor-prefixes.txt
        src/main.c
    test/                         The test suite. You do not edit this.
```

Exercise Set 4 has no code. Build and run each application from the repository root, after
`source env.sh`:

```bash
west build -p -b native_sim/native/64 lectures/L02/exercises/exercise1
west build -t run
```

Then, from the repository root:

```bash
make test               # Every lecture's suite, against your code.
```

or, for this lecture alone:

```bash
python3 ci/suite.py lectures/L02               # This lecture's suite, against your code.
python3 ci/suite.py --solutions lectures/L02   # The same tests, against the published solutions.
```

A set you have not started is reported as `SKIP`, with the file the tests are waiting for. Once
all of the set's files exist its tests are built and run; there is nothing to register. Every set
is built twice, the second time with an extra overlay from `test/`, so expect two results per set.
Each build goes into a directory of its own under `build-ci/test/L02/`.

**Each application grows through its set, and the tests check where it ends up**: Exercise 1.2's
blinking, Exercise 2.3's finger, Exercise 3.3's table. Until you reach the end of a set, expect its
tests to report what is still missing.

**Your work here is not committed.** The repository's `.gitignore` ignores everything in this
directory except this README and `test/`, so `git status` stays quiet and pulling new course
material never collides with your files. Use `git add -f` for a file you do want to commit.

Reference solutions are published in `appendix/solutions` a few days after the lecture. Try each
exercise before you read them.
