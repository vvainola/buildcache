-- match(echo)

-------------------------------------------------------------------------------
-- This is a minimal wrapper that caches stdout from the 'echo' command.
-------------------------------------------------------------------------------

function get_program_id ()
  -- We use the full path to the executable as a program identifier.
  return ARGS[1]
end

