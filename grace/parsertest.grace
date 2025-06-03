dialect "grast"

// Replace this with a copy of the parser to run it on test.grace.
// If a version with the AST module inlined (e.g. using -i on the Java version),
// the result will be printable. If it is not inlined, the result will be executable.
def program = objCons(nil, nil)

def parser = run(program)

// Load the source code to parse:
def contents = getFileContents "test.grace"

def result = requestMethod "parse" on(parser) args(lifter.string(contents))

// When the AST module is inlined, the objects are stringifiable:
print(stringify(result))

// When the AST module is mocked (from not being inlined), the code can
// be executed exactly the same way that the program is above.
//run(result)

// Produce the serialised AST of the parser alone with:
//    java -classpath java nz.mwh.wg.Start -p parser.grace
// or with the AST module inlined with:
//    java -classpath java nz.mwh.wg.Start -p -i parser.grace
