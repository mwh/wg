import "parser" as parser
import "objects" as objects
import "nodes" as nodes

def code = getFileContents "test.grace"
def node = parser.parse(code)

node.evaluate(nodes.evaluationContext(objects.terminalObject))
