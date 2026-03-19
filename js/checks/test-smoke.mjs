// Minimal smoke test
import { GraceNumber, GraceString, GraceBoolean, GRACE_DONE, pending } from '../grace/runtime.js';
import { n0M, s0L, l0R, o0C, b1K, nil, c0N, o1N, c2N, d3F, v4R, m0D, p0T, r3T, a5N, i0D } from '../grace/concise.js';
import { evalNode } from '../grace/evaluate.js';
import { makePrelude, makeInitialContext } from '../grace/prelude.js';

const prelude = makePrelude({ print: s => console.log('[print]', s) });
const ctx = makeInitialContext(prelude, new Map());

function runSteps(step) {
    let s = step;
    while (s !== null && s !== undefined) s = s.go();
}

let result;
let passed = 0;
let failed = 0;

function check(name, cond) {
    if (cond) {
        console.log(`  PASS: ${name}`);
        passed++;
    } else {
        console.error(`  FAIL: ${name}`);
        failed++;
    }
}

// Test: numLit
runSteps(evalNode(n0M(42), ctx, (val) => { result = val; return null; }));
check('numLit 42', result instanceof GraceNumber && result.value === 42);

// Test: strLit
runSteps(evalNode(s0L("hello"), ctx, (val) => { result = val; return null; }));
check('strLit "hello"', result instanceof GraceString && result.value === 'hello');

// Test: true lexical request
runSteps(evalNode(l0R('true', nil, nil), ctx, (val) => { result = val; return null; }));
check('true', result === GraceBoolean.TRUE);

// Test: false lexical request
runSteps(evalNode(l0R('false', nil, nil), ctx, (val) => { result = val; return null; }));
check('false', result === GraceBoolean.FALSE);

// Test: if(1)then(1) - true branch
const ifThenNode = l0R('if(1)then(1)', c2N(l0R('true', nil, nil), b1K(nil, o1N(n0M(99)))), nil);
runSteps(evalNode(ifThenNode, ctx, (val) => { result = val; return null; }));
check('if(true)then{99}', result instanceof GraceNumber && result.value === 99);

// Test: if(1)then(1) - false branch (should return done)
const ifThenFalse = l0R('if(1)then(1)', c2N(l0R('false', nil, nil), b1K(nil, o1N(n0M(99)))), nil);
runSteps(evalNode(ifThenFalse, ctx, (val) => { result = val; return null; }));
check('if(false)then{99} = done', result === GRACE_DONE);

// Test: if(1)then(1)else(1)
const ifElse = l0R('if(1)then(1)else(1)',
    c0N(l0R('false', nil, nil), c2N(b1K(nil, o1N(n0M(1))), b1K(nil, o1N(n0M(2))))), nil);
runSteps(evalNode(ifElse, ctx, (val) => { result = val; return null; }));
check('if(false)then{1}else{2}', result instanceof GraceNumber && result.value === 2);

// Test: object constructor with def
const objNode = o0C(o1N(d3F("x", nil, nil, n0M(7))), nil);
runSteps(evalNode(objNode, ctx, (val) => { result = val; return null; }));
check('object{def x=7} creates object', result && result._methods && 'x' in result._methods);

// Test: read the def back out
const obj = result;
let xVal;
runSteps(obj.requestMethod(ctx, (v) => { xVal = v; return null; }, 'x', []));
check('object.x == 7', xVal instanceof GraceNumber && xVal.value === 7);

// Test: print
runSteps(evalNode(l0R('print(1)', o1N(s0L('smoke test')), nil), ctx, (val) => { result = val; return null; }));
check('print returns done', result === GRACE_DONE);

// Test: GraceNumber arithmetic
let numResult;
const n = new GraceNumber(10);
runSteps(n.requestMethod(ctx, (v) => { numResult = v; return null; }, '+(1)', [new GraceNumber(5)]));
check('10 + 5 == 15', numResult instanceof GraceNumber && numResult.value === 15);

// Test: GraceRange
const r = new GraceNumber(1);
let rangeResult;
runSteps(r.requestMethod(ctx, (v) => { rangeResult = v; return null; }, '..(1)', [new GraceNumber(5)]));
const { GraceRange } = await import('../grace/runtime.js');
check('1..5 is GraceRange', rangeResult instanceof GraceRange);

// Test: while loop
let counter = 0;
const whileNode = l0R('while(1)do(1)', c2N(
    b1K(nil, o1N(l0R('false', nil, nil))),   // condition: always false -> immediate exit
    b1K(nil, o1N(n0M(0)))
), nil);
runSteps(evalNode(whileNode, ctx, (val) => { result = val; return null; }));
check('while(false)do{} = done', result === GRACE_DONE);

console.log(`\n${passed} passed, ${failed} failed`);
if (failed > 0) process.exit(1);
