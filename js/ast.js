
export class ObjectConstructor {
    constructor(body) {
        this.body = body;
    }
}

export class MethodDecl {
    constructor(parts, returnType, annotations, body) {
        this.parts = parts;
        this.returnType = returnType[0];
        this.annotations = annotations;
        this.body = body;
    }
}

export class DeclarationPart {
    constructor(name, params) {
        this.name = name;
        this.args = params;
    }
}

export class IdentifierDeclaration {
    constructor(name, type) {
        this.name = name;
        this.type = type[0];
    }

}

export class LexicalRequest {
    constructor(parts) {
        this.parts = parts;
    }
}

export class RequestPart {
    constructor(name, args) {
        this.name = name;
        this.args = args;
    }
}

export class ExplicitRequest {
    constructor(receiver, parts) {
        this.receiver = receiver;
        this.parts = parts;
    }
}

export class DefDecl {
    constructor(name, type, annotations, value) {
        this.name = name;
        this.type = type[0];
        this.annotations = annotations;
        this.value = value;
    }
}

export class VarDecl {
    constructor(name, type, annotations, value) {
        this.name = name;
        this.type = type[0];
        this.annotations = annotations;
        this.value = value[0];
    }
}

export class NumberNode {
    constructor(value) {
        this.value = value;
    }
}

export class StringNode {
    constructor(value) {
        this.value = value;
    }
}

export class Block {
    constructor(params, body) {
        this.params = params;
        this.body = body;
    }
}

export class Comment {
    constructor(value) {
        this.text = value;
    }
}



