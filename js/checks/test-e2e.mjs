// Test more complex Grace programs
import { GraceInterpreter } from '../grace/grace.js';

const interp = new GraceInterpreter({
    print: s => console.log('[Grace]', s),
});
await interp.init();

let passed = 0, failed = 0;

async function test(name, code, expectedOutputs=[]) {
    const outputs = [];
    const localInterp = new GraceInterpreter({
        print: s => outputs.push(s),
    });
    await localInterp.init();
    try {
        await localInterp.runString(code, name);
        if (expectedOutputs.length > 0) {
            const match = expectedOutputs.every((exp, i) => outputs[i] === exp);
            if (match) {
                console.log(`  PASS: ${name}`);
                passed++;
            } else {
                console.log(`  FAIL: ${name} — expected ${JSON.stringify(expectedOutputs)}, got ${JSON.stringify(outputs)}`);
                failed++;
            }
        } else {
            console.log(`  PASS: ${name} (output: ${JSON.stringify(outputs)})`);
            passed++;
        }
    } catch(e) {
        console.log(`  FAIL: ${name} — ${e.message}`);
        if (e.stack) console.log('   ', e.stack.split('\n').slice(0,4).join('\n    '));
        failed++;
    }
}

// Basic output
await test('print string', 'print("hello")', ['hello']);
await test('print number', 'print(42.asString)', ['42']);
await test('arithmetic', 'print((2 + 3).asString)', ['5']);
await test('string concat', 'print("Hello" ++ " " ++ "World")', ['Hello World']);

// Variables
await test('var mutation', `
var x := 1
x := x + 1
print(x.asString)
`, ['2']);

// Def
await test('def', `
def x = 42
print(x.asString)
`, ['42']);

// If/else
await test('if then else', `
def x = 5
if (x > 3) then {
    print("big")
} else {
    print("small")
}
`, ['big']);

// While loop
await test('while loop', `
var i := 0
var sum := 0
while { i < 5 } do {
    i := i + 1
    sum := sum + i
}
print(sum.asString)
`, ['15']);

// Object
await test('object with methods', `
def counter = object {
    var count := 0
    method increment {
        count := count + 1
    }
    method value {
        count
    }
}
counter.increment
counter.increment
counter.increment
print(counter.value.asString)
`, ['3']);

// Blocks
await test('block with apply', `
def double = { x -> (x + x) }
print(double.apply(5).asString)
`, ['10']);

// Recursive factorial using object
await test('factorial via method', `
def math = object {
    method factorial(n) {
        if (n <= 1) then {
            1
        } else {
            n * (factorial(n - 1))
        }
    }
}
print(math.factorial(5).asString)
`, ['120']);

// String operations
await test('string size', `
def s = "hello"
print(s.size.asString)
`, ['5']);

await test('string at', `
def s = "hello"
print(s.at(1))
`, ['h']);

// Range
await test('range for loop', `
var sum := 0
for (1..5) do { i -> sum := (sum + i) }
print(sum.asString)
`, ['15']);

console.log(`\n${passed} passed, ${failed} failed`);
