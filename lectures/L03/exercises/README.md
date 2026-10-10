# L03 Exercises - Your Workspace

Write your solutions to the exercises in [Appendix B](../appendix/b_exercises.md) here, one
application per exercise set, with the file names below. That is all it takes for `make test` to
find them:

```text
lectures/L03/exercises/
    exercise1/                    Exercise Set 1: who runs when
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise2/                    Exercise Set 2: a timer and the system work queue
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise3/                    Exercise Set 3: debouncing
        CMakeLists.txt
        prj.conf
        app.overlay
        src/main.c
    exercise4/                    Exercise Set 4: stop spinning
        CMakeLists.txt
        prj.conf
        src/main.c
    test/                         The test suite. You do not edit this.
```

Exercise Set 5 has no code. Build and run each application from the repository root, after
`source env.sh`:

```bash
west build -p -b native_sim/native/64 lectures/L03/exercises/exercise1
./build/zephyr/zephyr.exe -stop_at=2 -no-rt
```

Then, from the repository root:

```bash
make test               # Every lecture's suite, against your code.
```

or, for this lecture alone:

```bash
python3 ci/suite.py lectures/L03               # This lecture's suite, against your code.
python3 ci/suite.py --solutions lectures/L03   # The same tests, against the published solutions.
```

A set you have not started is reported as `SKIP`, with the file the tests are waiting for. Once
all of the set's files exist its tests are built and run; there is nothing to register. Each
application is built in a directory of its own under `build-ci/test/L03/`, and the first build of
each takes about half a minute; later runs rebuild only what changed.

**Each application grows through its set, and the tests check where it ends up**: Exercise 1.3's
absolute timeouts, Exercise 2.2's double submission and stop function, Exercise 3.2's debounced
presses, and Exercise 4.2's or 4.3's blocking wait. Until you reach the end of a set, expect its
tests to report what is still missing. The program you start Exercise Set 4 with never stops, and
the suite needs 30 seconds to give up on it.

**Your work here is not committed.** The repository's `.gitignore` ignores everything in this
directory except this README and `test/`, so `git status` stays quiet and pulling new course
material never collides with your files. Use `git add -f` for a file you do want to commit.

Reference solutions are published in `appendix/solutions` a few days after the lecture. Try each
exercise before you read them.
