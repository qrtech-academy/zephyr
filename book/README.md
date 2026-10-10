# The Book
The course typeset as a book with LuaLaTeX: six chapters, one per lecture, then the two
self-assessment papers as appendices. The examples and the solutions to the exercises stay in the
repository.

---

## Building it

```bash
sudo apt -y install make texlive-luatex texlive-latex-extra fonts-texgyre fonts-texgyre-math \
                    fonts-dejavu-core
make -C book                   # Writes book/zephyr-rtos.pdf, dated today.
make -C book VERSION=v1.2.3    # The same, with the version on the title page.
make -C book png               # Redraw the lectures' copies of the figures.
make -C book clean             # Removes book/build/ and the PDF.
```

The build runs LuaLaTeX twice, so the contents and the cross-references settle, then prints any
overfull or underfull lines and LaTeX warnings it found, and fails if a reference is left
undefined. Nothing here needs the Zephyr workspace: the book is built from its own sources, and
`make setup` is not required.

---

## Releasing a new edition
The PDF is committed, as `book/zephyr-rtos.pdf`, so the repository always holds a readable copy;
rebuild it with `make -C book` and commit it along with any change to the book. Each edition is
also published as a GitHub release, with its version on the title page. The book shares the
repository's version namespace, so push a `vx.y.z` tag:

```bash
git tag v1.2.3
git push origin v1.2.3
```

The [Book workflow](../.github/workflows/book.yml) then builds the PDF with the tag on its title
page and attaches it to a release of the same name. Because the namespace is shared, every version
tag publishes an edition of the book, whether or not the book itself changed.

The README's download link points at the committed PDF in `main`, not at a release, so it always
serves the newest build and never needs updating.

---

## What is where

```text
book.tex                The book: front matter and six chapters, in order.
zephyrbook.sty          Every visual decision: page, type, colours, code blocks, exercises.
zephyrbook.lua          How \code{...} typesets inline code (#, \n and line breaks).
figures/                The two figures, drawn in TikZ, and png.tex, which renders one as a
                        lecture's PNG.
front/                  Title pages and preface.
chapters/NN/            Chapter NN: chapter.tex (the opener), one file per appendix of lecture
                        LNN, summary.tex (the review) and exercises.tex.
back/exam/              Appendices A to E: the two papers and their model answers, from exam/.
```

Every `.tex` file typeset from course material starts with a comment naming its source, for
example:

```tex
% Section 1.2, from lectures/L01/appendix/b_zephyr_and_the_build.md.
```

---

## Updating the content
The course material is the source of truth, and the book follows it. **A chapter's text is a
typeset copy of its lecture's markdown.** When you change a lecture appendix, make the same change
in the `.tex` file whose header names it. The same holds for `exam/*.md` and `back/exam/`. The book
differs from the lectures in four ways, all of them deliberate:
* It addresses a reader rather than a participant: *this chapter*, not *this lecture*, and *you*
  rather than *participants*.
* A lecture's appendices are a chapter's sections, so a reference to *Appendix B* is `\secref`,
  and a reference to *L03* is `Chapter~\ref{c3:ch}`.
* The exercises are renumbered within each chapter, so Exercise 2.1 of a lecture may be
  Exercise~1.4 of a chapter. Always refer to one by its label, `\ref{c1:ex:2.1}`, which keeps the
  lecture's numbering.
* The example programs are named, not printed. They change with the course; the book points at
  `lectures/LNN/examples` instead of copying them.

A few conventions, so an edit reads like the rest of the book:
* Code blocks: `ccode` (C), `shell`, `cmakecode` (CMakeLists.txt), `dtscode` (devicetree),
  `kconfigcode` (Kconfig options and `prj.conf`), `yamlcode` (bindings, `sample.yaml`), and
  `console` (program output, directory trees, plain text).
* Inline code: `\code{...}`, written exactly as in the source. Inside it, write `\%` for `%`,
  `\{` or `\}` for a brace, `\\` for a backslash, and `\#` for `#`. File names use `\file{...}`,
  which breaks at a slash.
* References: `\secref{c2:app:a}` is the section typeset from appendix A of L02,
  `Chapter~\ref{c3:ch}` is a chapter, and `Exercise~\ref{c4:ex:3.1}` is exercise 3.1 of L04.
* Exercises: `\exerciseset{Title}` for an exercise set, `\exercise{Title}{Code}` or
  `\exercise[fillaccent2]{Title}{Reflection}` for one exercise, and `\task{Tasks}` or
  `\task{Reflection}` for the parts of one.
* A new lecture appendix is a new file in `chapters/NN/`, `\input` from that chapter's
  `chapter.tex`.
* The papers in `back/exam/` use `\question{Title}{Marks}`, `\qpart{a}`, `\worth{N}` and
  `\credit{...}`; the answers use `\answersection{N}{Title}{Marks}` and `\answerpart{a}{N}`, and
  each answers file sets `\answerprefix` to its own label prefix. **`\worth` and `\credit` must
  follow their text directly**, with no blank line between: after a blank line TeX is in vertical
  mode and the build stops with `You can't use \unskip in vertical mode`.
* Quotes in a listing are ASCII. GCC prints `‘name’`, but `listings` typesets those two
  characters before the name they quote, whatever they are mapped to, so a code block that quotes a
  compiler message uses `'name'`. Inline `\code{...}` renders them correctly, and the lectures keep
  the real output.

**The figures are the lectures' figures.** `figures/build_pipeline.tex` and
`figures/priority_inversion.tex` are drawn in TikZ, in the book's own fonts and inks, and
`make -C book png` renders each one into the lecture that shows it
(`lectures/L01/appendix/images/` and `lectures/L04/appendix/images/`). Edit the `.tex` file, run
that target, and the lecture and the book stay one drawing.

The solutions to the exercises are not in the book. Each chapter's exercises say where they are
published in the repository (`lectures/LNN/appendix/solutions`).

---

## License
The book, its text and figures and the PDF built from them, is licensed under
[CC BY-NC-SA 4.0](../LICENSE-CONTENT), like the course material it is typeset from. This
directory's build files (`zephyrbook.sty`, `zephyrbook.lua`, `Makefile`) are released under the
repository's [MIT License](../LICENSE), like all the code in the course, and so may the code
examples printed in the book be used.
