#!/usr/bin/env node
/**
 * Grace interpreter runner for Node.js
 *
 * Usage:
 *   node run.mjs program.grace
 *   node run.mjs --eval 'print "Hello, world!"'
 */

import { GraceInterpreter } from './grace/grace.js';
import { readFile } from 'node:fs/promises';
import { resolve } from 'node:path';

const args = process.argv.slice(2);

if (args.length === 0) {
    console.error('Usage: node run.mjs <file.grace>');
    console.error('       node run.mjs --eval "grace code"');
    process.exit(1);
}

const interp = new GraceInterpreter({
    print: s => process.stdout.write(s + '\n'),
    readFile: (p) => readFile(resolve(process.cwd(), p), 'utf8'),
});

try {
    if (args[0] === '--eval') {
        await interp.init();
        await interp.runString(args.slice(1).join(' '), '<eval>');
    } else {
        await interp.init();
        await interp.runFile(resolve(process.cwd(), args[0]));
    }
} catch (e) {
    if (e._isGraceException) {
        // Uncaught Grace exception: print the message to stdout (standard Grace behaviour)
        process.stdout.write(e.message + '\n');
        process.exit(1);
    } else if (e.graceCallStack) {
        console.error('Grace error:', e.message);
        console.error(e.graceCallStack);
        process.exit(1);
    } else {
        console.error(e);
        process.exit(1);
    }
}
