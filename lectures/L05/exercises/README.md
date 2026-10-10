# L05 Exercises - Your Workspace

Write your solutions to the exercises in [Appendix C](../appendix/c_exercises.md) here, one
directory per exercise set, with the file names below. That is all it takes for `make test` to
find them:

```text
lectures/L05/exercises/
    exercise1/                         Exercise Set 1: a module and a device
        zephyr/module.yml
        CMakeLists.txt
        Kconfig
        dts/bindings/qacademy,lumo.yaml
        dts/bindings/vendor-prefixes.txt
        src/lumo_reg.h
        src/lumo.c
    exercise2/                         Exercise Set 2: reading light. A copy of exercise1, grown.
        (the same files)
    exercise3/                         Exercise Set 3: the gain. A copy of exercise2, grown.
        (the same files)
    exercise4/                         Exercise Set 4: an application, using exercise3's module
        CMakeLists.txt
        prj.conf
        app.overlay
        src/main.c
    exercise5/                         Exercise Set 5: tests of your own, of exercise3's driver
        CMakeLists.txt
        prj.conf
        app.overlay
        testcase.yaml
        src/main.c
    test/                              The test suite, and the Lumo emulator. You do not edit this.
```

Sets 4 and 5 use the module in `exercise3`, so their tests also wait for `exercise3/src/lumo.c`.
Build and run from the repository root, after `source env.sh`, with the emulator added on the
command line; Appendix C gives the exact commands. Then, from the repository root:

```bash
make test               # Every lecture's suite, against your code.
```

or, for this lecture alone:

```bash
python3 ci/suite.py lectures/L05               # This lecture's suite, against your code.
python3 ci/suite.py --solutions lectures/L05   # The same tests, against the published solutions.
```

A set you have not started is reported as `SKIP`, with the file the tests are waiting for. Once
all of the set's files exist its tests are built and run; there is nothing to register. The tests
are built in directories of their own under `build-ci/test/L05/`, a few at a time, and the first
run takes a few minutes; later runs rebuild only what changed.

**Each set's tests check where the set ends up**: Set 1's init, Set 2's conversion, Set 3's gain.
Sets 2 and 3 also run the tests of the sets before them, so a copy that breaks what already worked
is caught.

**Your work here is not committed.** The repository's `.gitignore` ignores everything in this
directory except this README and `test/`, so `git status` stays quiet and pulling new course
material never collides with your files. Use `git add -f` for a file you do want to commit.

Reference solutions are published in `appendix/solutions` a few days after the lecture. Try each
exercise before you read them.
