// Test bootstrapping the Grace parser from modules.js
import { GraceInterpreter } from '../grace/grace.js';

console.log('Testing Grace bootstrap...');

const interp = new GraceInterpreter({
    print: s => console.log('[Grace]', s),
});

console.log('Initialising interpreter (loading lexer/parser/ast modules)...');
await interp.init();
console.log('Interpreter ready.');

// Test 1: parse a very simple Grace expression
console.log('\nTest 1: Parsing "print(42)"...');
try {
    const concise = await interp.parseToString('print(42)', 'test');
    console.log('Concise output:', concise.slice(0, 100) + (concise.length > 100 ? '...' : ''));
    console.log('PASS: parsing produced output');
} catch(e) {
    console.error('FAIL:', e.message);
    if (e.stack) console.error(e.stack.split('\n').slice(0, 5).join('\n'));
}

// Test 2: run a simple print
console.log('\nTest 2: Running print("Hello from Grace!")...');
try {
    await interp.runString('print("Hello from Grace!")', 'test2');
    console.log('PASS: runString completed');
} catch(e) {
    console.error('FAIL:', e.message);
    if (e.stack) console.error(e.stack.split('\n').slice(0, 5).join('\n'));
}

// Test 3: arithmetic  
console.log('\nTest 3: Running arithmetic...');
try {
    await interp.runString('print((3 + 4).asString)', 'test3');
    console.log('PASS: arithmetic runString completed');
} catch(e) {
    console.error('FAIL:', e.message);
    if (e.stack) console.error(e.stack.split('\n').slice(0, 5).join('\n'));
}

console.log('\nBootstrap tests done.');
