dialect "grast"

// Replace this with a copy of the parser to run it on test.grace:
def program = objCons(nil, nil)

def parser = run(program)

// Load the source code to parse:
def contents = getFileContents "test.grace"

def result = requestMethod "parse" on(parser) args(lifter.string(contents))
print(stringify(result))
