-- zephyrbook.lua - the one piece of the design that TeX macros cannot do cleanly.
--
-- \code{...} typesets code exactly as it is written in the lectures. \detokenize gets most of the
-- way, but leaves two artifacts that C runs into constantly: it doubles every #, so #include
-- would print as ##include, and it puts a space after every control word, so "\n" would print as
-- "\n ". Both are undone here.
--
-- A line break is then allowed at the places Zephyr's names and paths are long enough to need
-- one: after a comma, after a slash in a path such as native_sim/native/64, and, as a last
-- resort, after an underscore, so that CONFIG_NATIVE_SIM_SLOWDOWN_TO_REAL_TIME can break rather
-- than run into the margin. The underscore is penalised ten times as heavily as the others, so a
-- name is only ever split when nothing else will do.

local catcode_other = -2

function zephyrbook_code(s)
  s = s:gsub("#+", "#")
  s = s:gsub("(\\%a+) ", "%1")
  -- \%, \{ and \} are how a literal percent sign or an unbalanced brace has to be written inside
  -- a TeX argument, \# is how a # has to be written in a heading or a caption, and \\ is a
  -- lone backslash.
  s = s:gsub("\\([%%{}#\\])", "%1")
  -- Each tex.sprint is read as a line of its own and TeX skips the spaces a line starts with, so
  -- no chunk may begin with one. The penalty after a chunk says how unwelcome a break there is.
  local start = 1
  while true do
    local i, penalty = nil, nil
    for pattern, p in pairs({["/[^ ]"] = 100, [",[^ ]"] = 100, ["_[^ ]"] = 1000}) do
      local k = s:find(pattern, start)
      if k and (not i or k < i) then i, penalty = k, p end
    end
    if not i then break end
    tex.sprint(catcode_other, s:sub(start, i))
    tex.sprint("\\penalty" .. penalty .. " ")
    start = i + 1
  end
  tex.sprint(catcode_other, s:sub(start))
end
