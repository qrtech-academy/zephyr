# L06 Exercises - Your Workspace

Write your solutions to the exercises in [Appendix C](../appendix/c_exercises.md) here, one
application per exercise set, with the file names below. That is all it takes for `make test` to
find them:

```text
lectures/L06/exercises/
    exercise1/                    Exercise Set 1: shell commands of your own
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise2/                    Exercise Set 2: a setpoint that survives a restart
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise3/                    Exercise Set 3: fatal errors
        CMakeLists.txt
        prj.conf
        src/main.c
        src/fatal.c
    exercise4/                    Exercise Set 4: sanitizers
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise5/                    Exercise Set 5: sockets
        CMakeLists.txt
        prj.conf
        src/main.c
    test/                         The test suite. You do not edit this.
```

Build and run each application from the repository root, after `source env.sh`:

```bash
west build -p -b native_sim/native/64 lectures/L06/exercises/exercise1
west build -t run
```

Then, from the repository root:

```bash
make test               # Every lecture's suite, against your code.
```

or, for this lecture alone:

```bash
python3 ci/suite.py lectures/L06               # This lecture's suite, against your code.
python3 ci/suite.py --solutions lectures/L06   # The same tests, against the published solutions.
```

A set you have not started is reported as `SKIP`, with the file the tests are waiting for. Once
all of the set's files exist its tests are built and run; there is nothing to register. Each
application is built in a directory of its own under `build-ci/test/L06/`, and the first build of
each takes about half a minute; later runs rebuild only what changed.

**Each application grows through its set, and the tests check where it ends up**: Exercise 1.2's
validation, Exercise 2.2's reset, Exercise 3.3's assertion, Exercise 4.3's fixed program. Until you
reach the end of a set, expect its tests to report what is still missing.

**When a sanitizer build fails, the suite shows the start of the report**: the error, the access,
the first stack frame, and AddressSanitizer's `SUMMARY:` line. For the whole report, build with
`-DCONFIG_ASAN=y -DCONFIG_UBSAN=y` and run the program yourself.

**Your work here is not committed.** The repository's `.gitignore` ignores everything in this
directory except this README and `test/`, so `git status` stays quiet and pulling new course
material never collides with your files. Use `git add -f` for a file you do want to commit.

Reference solutions are published in `appendix/solutions` a few days after the lecture. Try each
exercise before you read them.
