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
let theSerialisedAST

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
    document.getElementById('ast').value = cont.result
    theAST = cont.result
    let end = performance.now()
    status("Ready. Parse time: " + Math.round(end - start) + "ms")
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
    status("Ready. Run time: " + Math.round(end - start) + "ms")
})

document.getElementById('java').addEventListener('click', async () => {
    if (!theAST)
        await doParse()
    let text = theAST
    let f = await fetch("flat-template.java")
    let base = await f.text()
    document.getElementById('ast').value = base + `class TheProgram extends ASTConstructors {
    @SuppressWarnings("unchecked")
    public static final ASTNode program = ${text};
}`
    let blob = new Blob([document.getElementById('ast').value], {type: "text/plain"})
    let url = URL.createObjectURL(blob)
    let a = document.createElement('a')
    a.href = url
    a.download = "GraceProgram.java"
    a.textContent = 'Download GraceProgram.java'
    document.getElementById('status').replaceChildren(a)
})

document.getElementById('haskell').addEventListener('click', async () => {
    if (!theAST)
        await doParse()
    let text = theAST
    let f = await fetch("flat-template.hs")
    let base = await f.text()
    document.getElementById('ast').value = base + `\nprogram = ${text}\n`
    let blob = new Blob([document.getElementById('ast').value], {type: "text/plain"})
    let url = URL.createObjectURL(blob)
    let a = document.createElement('a')
    a.href = url
    a.download = "GraceProgram.hs"
    a.textContent = 'Download GraceProgram.hs'
    document.getElementById('status').replaceChildren(a)
})

document.getElementById('c_flat').addEventListener('click', async () => {
    if (!theAST)
        await doParse()
    let text = theAST
    let f = await fetch("flat-template.c")
    let base = await f.text()
    document.getElementById('ast').value = base.replace(/program = .*/, 'program = ' + text + ';')
    let blob = new Blob([document.getElementById('ast').value], {type: "text/plain"})
    let url = URL.createObjectURL(blob)
    let a = document.createElement('a')
    a.href = url
    a.download = "GraceProgram.c"
    a.textContent = 'Download GraceProgram.c'
    document.getElementById('status').replaceChildren(a)
})

document.getElementById('excel').addEventListener('click', async () => {
    if (!theAST)
        await doParse();
    let hzip = (await import('./hzip.js'))['default'];
    let text = theAST
    let worksheetF = fetch('excel-book-template.xlsx');
    let f = await fetch("excel-sheet-template.xml")
    let base = await f.text()
    let sheetDoc = new DOMParser().parseFromString(base, 'text/xml');
    sheetDoc.getElementsByTagName('f')[0].textContent = text;
    let sheet1XML = new XMLSerializer().serializeToString(sheetDoc);
    let worksheetR = await worksheetF;
    let zip = new hzip.ZipFile(await worksheetR.arrayBuffer());
    await zip.entries['xl/worksheets/sheet1.xml'].put(new Blob([sheet1XML]));
    let blob = await zip.asBlob();
    let url = URL.createObjectURL(blob)
    let a = document.createElement('a')
    a.href = url
    a.download = "GraceSpreadsheet.xlsx"
    a.textContent = 'Download GraceSpreadsheet.xlsx'
    document.getElementById('status').replaceChildren(a)
    document.getElementById('ast').value = 'Download the spreadsheet and recalculate (Ctrl-Alt-F9) upon opening.';
})


document.getElementById('javascript').addEventListener('click', async () => {
    if (!theAST)
        await doParse()
    let text = theAST
    text = `
let cont = {
    go(v) {return null},
    of(v) {
        this.result = v;
        return this;
    },
    toString() {return "END"}
}
function evaluate(cont) {
    cont = cont.go(undefined);
    if (!cont)
        return
    setTimeout(() => evaluate(cont), 0)
}

const program = ${text}

evaluate(evaluateModule(cont, program))
`
    let f = await fetch("ast.js")
    let astText = await f.text()
    f = await fetch("evaluator.js")
    let evalText = await f.text()
    f = await fetch("program.js")
    let programText = await f.text()
    programText = programText.replace(/const program = .*/g, "")
    let base = '// Run as `node GraceProgram.js`\n' + astText + evalText + programText
    base = base.replace(/^import .*/gm, "")
    base = base.replace(/^export default .*/gm, "")
    base = base.replace(/^export /gm, "")
    base = base.replace(/ast\./g, "")
    document.getElementById('ast').value = base + text
    let blob = new Blob([document.getElementById('ast').value], {type: "text/plain"})
    let url = URL.createObjectURL(blob)
    let a = document.createElement('a')
    a.href = url
    a.download = "GraceProgram.js"
    a.textContent = 'Download GraceProgram.js'
    document.getElementById('status').replaceChildren(a)
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
