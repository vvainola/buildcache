-- match(.*cl6x.*)

-------------------------------------------------------------------------------
-- This is a wrapper for the TI C6000 DSP compiler.
-------------------------------------------------------------------------------

require_std("io")
require_std("os")
require_std("string")
require_std("table")


-------------------------------------------------------------------------------
-- General helper functions (the corresponding C++ functions in BuildCache
-- should probably be exposed to Lua instead of re-implementing them in Lua).
-------------------------------------------------------------------------------

local function make_escaped_cmd (args)
  -- Convert the argument list to a command string with each argument escaped.
  local cmd = ""
  for i, arg in ipairs(args) do
    arg = arg:gsub("\"", "\\\"")
    if arg:find(" ") then
      arg = "\"" .. arg .. "\""
    end
    if cmd:len() > 0 then
      cmd = cmd .. " "
    end
    cmd = cmd .. arg
  end
  return cmd
end

local function run (args)
  -- Run the command.
  local file = assert(io.popen(make_escaped_cmd(args), "r"))
  local result = {}
  result.std_out = file:read("*all")
  result.std_err = ""
  local close_result = {file:close()}
  result.return_code = close_result[3]
  return result
end

local function get_file_part (path)
  local pos = path:find("/[^/]*$")
  if pos == nil then
    pos = path:find("\\[^\\]*$")
  end
  return pos ~= nil and path:sub(pos + 1) or path
end


-------------------------------------------------------------------------------
-- Internal helper functions.
-------------------------------------------------------------------------------

local function starts_with (s, substr)
  return s:sub(1, #substr) == substr
end

local function make_preprocessor_cmd (args, preprocessed_file)
  local preprocess_args = {}

  -- Drop arguments that we do not want/need.
  local drop_next_arg = false
  for i, arg in ipairs(args) do
    local drop_this_arg = drop_next_arg
    drop_next_arg = false
    if (arg == "--compile_only") or
       starts_with(arg, "--output_file=") or
       starts_with(arg, "-pp") or
       starts_with(arg, "---preproc_") then
      drop_this_arg = true
    end
    if not drop_this_arg then
      table.insert(preprocess_args, arg)
    end
  end

  -- Append the required arguments for producing preprocessed output.
  table.insert(preprocess_args, "--preproc_only")
  table.insert(preprocess_args, "--output_file=" .. preprocessed_file)

  return preprocess_args
end


-------------------------------------------------------------------------------
-- Wrapper interface implementation.
-------------------------------------------------------------------------------

function preprocess_source ()
  -- Check if this is a compilation command that we support.
  local is_object_compilation = false
  local has_object_output = false
  for i, arg in ipairs(ARGS) do
    if arg == "--compile_only" then
      is_object_compilation = true
    elseif starts_with(arg, "--output_file=") then
      has_object_output = true
    elseif starts_with(arg, "--cmd_file=") or starts_with(arg, "-@") then
      error("Response files are currently not supported.")
    end
  end
  if (not is_object_compilation) or (not has_object_output) then
    error("Unsupported complation command.")
  end

  -- Run the preprocessor step.
  local preprocessed_file = os.tmpname()
  local preprocessor_args = make_preprocessor_cmd(ARGS, preprocessed_file)
  local result = run(preprocessor_args)
  if result.return_code ~= 0 then
    os.remove(preprocessed_file)
    error("Preprocessing command was unsuccessful.")
  end

  -- Read and return the preprocessed file.
  local f = assert(io.open(preprocessed_file, "rb"))
  local preprocessed_source = f:read("*all")
  f:close()
  os.remove(preprocessed_file)

  return preprocessed_source
end

function get_relevant_arguments ()
  local filtered_args = {}

  -- The first argument is the compiler binary without the path.
  table.insert(filtered_args, get_file_part(ARGS[1]))

  -- Note: We always skip the first arg since we have handled it already.
  local skip_next_arg = true
  for i, arg in ipairs(ARGS) do
    if not skip_next_arg then
      -- Generally unwanted argument (things that will not change how we go
      -- from preprocessed code to binary object files)?
      local first_two_chars = arg:sub(1, 2)
      local is_unwanted_arg = (first_two_chars == "-I") or
                              starts_with(arg, "--include") or
                              starts_with(arg, "--preinclude=") or
                              (first_two_chars == "-D") or
                              starts_with(arg, "--define=") or
                              starts_with(arg, "--c_file=") or
                              starts_with(arg, "--cpp_file=") or
                              starts_with(arg, "--output_file=")

      if not is_unwanted_arg then
        table.insert(filtered_args, arg)
      end
    else
      skip_next_arg = false
    end
  end

  return filtered_args
end

function get_program_id ()
  -- TODO(m): Add things like executable file size too.

  -- Get the help string from the compiler (it includes the version string).
  local result = run({ARGS[1], "--help"})
  if result.return_code ~= 0 then
    error("Unable to get the compiler version information string.")
  end

  return result.std_out
end

function get_build_files ()
  local files = {}
  local found_object_file = false
  local found_dep_file = false
  for i = 2, #ARGS do
    local next_idx = i + 1
    if starts_with(ARGS[i], "--output_file=") then
      if found_object_file then
        error("Only a single target object file can be specified.")
      end
      files["object"] = ARGS[i]:sub(15)
      found_object_file = true
    elseif starts_with(ARGS[i], "-ppd=") or starts_with(ARGS[i], "--preproc_dependency=") then
      if found_dep_file then
        error("Only a single dependency file can be specified.")
      end
      local eq = ARGS[i]:find("=")
      files["dep"] = ARGS[i]:sub(eq + 1)
      found_dep_file = true
    end
  end
  if not found_object_file then
    error("Unable to get the target object file.")
  end
  return files
end


