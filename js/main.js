import * as ast from "./ast.js"
import * as evaluator from "./evaluator.js"
import {evaluate as toJSAST} from "./program.js";

import program from "./program.js"

let cont = {
    go(v) {return null},
    of(v) {
        this.result = v;
        return this;
    },
    toString() {return "END"}
}

let output = document.getElementById('output')

function say(s) {
    let p = document.createElement('p')
    p.textContent = s
    output.appendChild(p)
}

function status(s) {
    document.getElementById('status').textContent = s
}

let theAST

document.getElementById('source').addEventListener('input', () => {
    theAST = null
})

document.getElementById('source').addEventListener('keydown', function(e) {
    if (e.key === "Tab") {
        e.preventDefault();
        this.setRangeText("    ", this.selectionStart, this.selectionStart, "end");
    } else if (e.key == "Backspace") {
        let start = this.selectionStart
        let end = this.selectionEnd
        if (start === end && start > 0 && this.value.substring(start - 4, start) === "    ") {
            this.setRangeText("", start - 3, start, "end")
        }
    
    }
})

async function doParse() {
    let start = performance.now()
    let text = document.getElementById('source').value
    status("Parsing input...")
    let cont = {
        go(v) {return null},
        of(v) {
            this.result = v;
            return this;
        },
        toString() {return "END"}
    }
    let r = parser.request(cont, {name: "parse(1)"}, [text])
    await evaluate(r)
    status("Stringifying AST...")
    let parseTree = cont.result
    let r2 = parseTree.request(cont, {name: "asString(0)"}, [])
    await evaluate(r2)
    document.getElementById('ast').textContent = cont.result
    theAST = cont.result
    let end = performance.now()
    status("Ready. Parse time: " + (end - start) + "ms")
}

document.getElementById('parse').addEventListener('click', async () => {
    await doParse()
})

document.getElementById('run').addEventListener('click', async () => {
    stepsTaken = 0
    document.getElementById('output').replaceChildren()
    if (!theAST) {
        await doParse()
    }
    let start = performance.now();
    status("Converting AST to JS...")
    let jsAST = toJSAST(theAST)
    status("Evaluating program...")
    let r = evaluator.evaluateModule(
        cont,
        jsAST)
    await evaluate(r)
    let end = performance.now()
    status("Ready. Run time: " + (end - start) + "ms")
})

let steps = document.getElementById('steps')
let r = evaluator.evaluateModule(
    cont,
    program)
let stepsTaken = 0
let maxSteps = 10000000

status("Loading parser...")
console.time("load parser")
await evaluate(r)
console.timeEnd("load parser")
status("Ready.")
let parser = cont.result

async function evaluate(cont) {
    return new Promise(resolve => evaluateContinuation(resolve, cont))
}

function evaluateContinuation(resolve, cont, expSteps=1000) {
    let r = cont
    let i
    for (i = 0; i < expSteps; i++) {
        stepsTaken++
        r = r.go(undefined);
        if (!r)
            break
    }
    if (stepsTaken > maxSteps) {
        console.log("max steps reached")
        say("Reached maximum step count")
        return
    }
    if (i < 100 || !r) {
        resolve()
    } else {
        console.log("continuing, expSteps", expSteps)
        setTimeout(() => evaluateContinuation(resolve, r, expSteps * 2), 0)
    }
}
