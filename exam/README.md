# Written Examinations

Two complete three-hour papers for **Zephyr RTOS**, with worked solutions.

```text
paper_a.md              Paper A, questions only. Hand this out.
paper_a_solutions.md    Paper A, model answers with marks.
paper_b.md              Paper B, questions only.
paper_b_solutions.md    Paper B, model answers with marks.
```

---

## What these are for

The course as written has **no exam and nothing is marked**. What you get instead is the exercise
sets after every lecture, each with a published solution, and the Evaluation questions that close
every lecture README.

These papers do not replace that. **They are here for you to test your own skills and knowledge,
nothing more.** They gate nothing, they are not a qualification, and no part of the course requires
them. Nothing in the repository depends on them, and neither `make examples` nor `make test` knows
they exist.

What they check is something the exercises cannot. Every exercise is done at a keyboard, with the
appendix open in the next tab and a build ready to say no. These papers ask what you can reconstruct
with none of that in front of you — which of the rules you have absorbed, and which you have merely
been looking up.

**Take one after the course is over.** Both papers draw on all six lectures, so sitting one partway
through examines material nobody has taught you yet, and the result says more about how far you have
read than about what you have understood.

---

## The two papers

Both cover the whole course, L01 to L06, at the same weighting. They share no question. Either can
be used alone; use both as a main sitting and a resit, or in alternate years.

| Question | Topic                                             | Lectures | Marks   |
| -------- | ------------------------------------------------- | -------- | ------- |
| 1        | The workspace and the build                       | L01      | 12      |
| 2        | Kconfig and logging                               | L01      | 12      |
| 3        | Devicetree, bindings and overlays                 | L02      | 13      |
| 4        | Threads, scheduling and time                      | L03      | 13      |
| 5        | Interrupt context, work queues and simulated time | L03      | 12      |
| 6        | Synchronization and communication                 | L04      | 13      |
| 7        | Writing a driver                                  | L05      | 13      |
| 8        | Subsystems and debugging                          | L06      | 12      |
|          |                                                   |          | **100** |

**Both papers mix prose with artifacts**, because the subject does. Roughly half the marks are for
explaining what a build decided and why; the other half are for reading or writing the things a
Zephyr application is made of: a `prj.conf`, a devicetree overlay, a binding, a driver, a thread.

The two papers differ in which half they lean on:

* **Paper A leans towards what the build hands you.** Its questions put an artifact in front of you
  — a build that aborted, a devicetree that produced the wrong macro, a trace of threads in the
  wrong order, a driver whose device is never ready — and ask what happened and what to change.
  Naming the cause is half the marks; saying what it does to the running program is the other half.
* **Paper B leans towards what you write.** It asks for a Kconfig menu, a binding and the overlay
  that instantiates it, a sampling design split between an interrupt and a work queue, the skeleton
  of a devicetree-instantiated driver, and a ztest suite for it.

Neither paper asks anything that a careful reader could not answer from the appendices. Where a
question turns on a rule that is easy to state and easy to get backwards — which file a Kconfig
value comes from last, why a log message that was filtered out is not in the binary, which context a
timer's expiry function runs in — the solution says so at length, because that is where the marks
and the understanding both are.

---

## Conventions the papers assume

Both papers state these in their own rubric, so a candidate never has to have read this file.

* **Zephyr 4.4 and the board `native_sim/native/64`**, built with the host's GCC, as in the course.
* **Code is marked on semantics, not syntax.** A missing `#include`, a missing semicolon or a brace
  in the wrong column costs nothing. A missing `device_is_ready()` check costs everything.
* **The course's house style is expected** where a question asks for it: Zephyr's own, meaning
  `snake_case`, the kernel's `k_` and `K_` spellings, application options prefixed `APP_`,
  `IS_ENABLED()` rather than `#ifdef` where both would work, and log modules rather than `printk()`
  in anything that is not a one-off.
* **The simulator's behaviour may be assumed** without deriving it: 100 ticks per second, a sleep
  that ends on the tick after the one it asked for, simulated time that only advances when every
  thread is waiting, immediate logging, and no thread analyzer, stack protection or userspace.
* **"State the consequence" means the consequence.** Naming a cause earns half the marks; saying
  what it does to the running program earns the other half. "This is wrong" scores nothing.
* **Where a question asks for a stated number of defects**, listing more is not penalised, but only
  that many are marked, so the strongest answers go first.

---

## Marking

Every solution is written to be marked by somebody who has read the appendices and does not
otherwise write Zephyr applications for a living, so each carries the reasoning rather than the
answer alone. Marks are shown per part.

Three conventions worth agreeing before a paper is marked:

* **Method carries the marks.** A correct diagnosis with a clumsy fix is worth more than a correct
  fix with no diagnosis. Several questions feed the next part, so an error should be followed
  through rather than penalised twice.
* **The named traps are worth full marks on their own.** A number of parts exist entirely to see
  whether a candidate walks into a specific mistake: a `CONFIG_` option misspelled in C rather than
  in `prj.conf`, a devicetree node that is `disabled` rather than absent, a `k_sleep()` in a timer's
  expiry function, a semaphore used as a lock, a driver that reads its configuration from
  `dev->data`, a busy loop on `native_sim`. Where a solution flags one of these, a candidate who
  walks into it loses those marks and no others.
* **The discussion parts are not decoration.** "State why" is where the course's actual content is,
  and a paper marked only on whether the code would build would pass a candidate who has understood
  none of it.

---
