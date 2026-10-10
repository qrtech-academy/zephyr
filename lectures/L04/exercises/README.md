# L04 Exercises - Your Workspace

Write your solutions to the exercises in [Appendix C](../appendix/c_exercises.md) here, one
application per exercise set, with the file names below. That is all it takes for `make test` to
find them:

```text
lectures/L04/exercises/
    exercise1/                    Exercise Set 1: readings in a queue
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise2/                    Exercise Set 2: priority inversion
        Kconfig
        CMakeLists.txt
        prj.conf
        mutex.conf
        src/main.c
    exercise3/                    Exercise Set 3: a battery on zbus
        CMakeLists.txt
        prj.conf
        src/main.c
    exercise4/                    Exercise Set 4: a garage door as a state machine
        CMakeLists.txt
        prj.conf
        src/door.h
        src/door.c
        src/main.c
    test/                         The test suite. You do not edit this.
```

Build and run each application from the repository root, after `source env.sh`:

```bash
west build -p -b native_sim/native/64 lectures/L04/exercises/exercise1
./build/zephyr/zephyr.exe -stop_at=3 -no-rt
```

Then, from the repository root:

```bash
make test               # Every lecture's suite, against your code.
```

or, for this lecture alone:

```bash
python3 ci/suite.py lectures/L04               # This lecture's suite, against your code.
python3 ci/suite.py --solutions lectures/L04   # The same tests, against the published solutions.
```

A set you have not started is reported as `SKIP`, with the file the tests are waiting for. Once
all of the set's files exist its tests are built and run; there is nothing to register. Each
application is built in a directory of its own under `build-ci/test/L04/`, and the first build of
each takes about half a minute; later runs rebuild only what changed.

**Each application grows through its set, and the tests check where it ends up**: Exercise 1.2's
queue that keeps the newest readings, Exercise 2.2's two builds, Exercise 3.2's subscriber, and
Exercise 4.2's program together with Exercise 4.1's state machine. Until you reach the end of a set,
expect its tests to report what is still missing.

**Your work here is not committed.** The repository's `.gitignore` ignores everything in this
directory except this README and `test/`, so `git status` stays quiet and pulling new course
material never collides with your files. Use `git add -f` for a file you do want to commit.

Reference solutions are published in `appendix/solutions` a few days after the lecture. Try each
exercise before you read them.
