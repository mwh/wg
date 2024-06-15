use constant charBackslash => "\\";
use constant charDollar => "\$";
use constant charDQuote => "\"";
use constant charLF => "\n";
use constant charCR => "\r";
use constant charLBrace => "{";
use constant charStar => "*";
use constant charTilde => "~";
use constant charBacktick => "`";
use constant charCaret => "^";
use constant charAt => "@";
use constant charPercent => "%";
use constant charAmp => "&";
use constant charHash => "#";
use constant charExclam => "!";

use Data::Dumper;


sub nil {
    my @arr = [];
    return \@arr;
}

sub cons {
    my $hd = shift;
    my $tl = shift;
    my @res = ($hd, @$tl);
    return \@res;
}

sub one {
    my @res = ($_[0]);
    return \@res;
}

sub objCons {
    my @body = shift;
    return { "type" => "ObjectConstructor", "body" => @body };
}

sub importStmt {
    my $source = shift;
    my $binding = shift;
    return { "type" => "ImportDeclaration", "source" => $source, "binding" => $binding };
}

sub identifierDeclaration {
    my $name = shift;
    my $typeAnnotation = shift;
    return { "type" => "binding", "name" => $name, "typeAnnotation" => $typeAnnotation[0] };
}

sub comment {
    return { "type" => "Comment", "value" => $_[0] };
}

sub defDec {
    my $name = shift;
    my $decType = shift;
    my $annotations = shift;
    my $value = shift;
    return { "type" => "DefDeclaration", "name" => $name, "decType" => $decType->[0], "annotations" => @$annotations, "value" => $value };
}

sub varDec {
    my $name = shift;
    my $decType = shift;
    my $annotations = shift;
    my $value = shift;
    return { "type" => "VariableDeclaration", "name" => $name, "decType" => $decType->[0], "annotations" => @$annotations, "value" => $value->[0] };
}

sub methDec {
    my $parts = shift;
    my $returnType = shift;
    my $annotations = shift;
    my $body = shift;
    return { "type" => "MethodDeclaration", "parts" => $parts, "returnType" => $returnType->[0], "annotations" => @$annotations, "body" => $body };
}

sub assn {
    my $left = shift;
    my $right = shift;
    return { "type" => "AssignmentExpression", "left" => $left, "right" => $right };
}

sub dotReq {
    my $receiver = shift;
    my $parts = shift;
    return { "type" => "ExplicitRequest", "receiver" => $receiver, "parts" => $parts };
}

sub lexReq {
    return { "type" => "LexicalRequest", "parts" => $_[0] };
}

sub interpStr {
    return { "type" => "InterpolatedString", "before" => $_[0], "expression" => $_[1], "after" => $_[2] };
}

sub returnStmt {
    return { "type" => "ReturnStatement", "argument" => $_[0] };
}

sub safeStr {
    return $_[0] . $_[1] . $_[2];
}

sub part {
    my $name = shift;
    my $parameters = shift;
    return { "type" => "Part", "name" => $name, "parameters" => @$parameters };
}

sub numLit {
    return { "type" => "NumericLiteral", "value" => $_[0] };
}

sub strLit {
    return { "type" => "StringLiteral", "value" => $_[0] };
}

sub block {
    my $parameters = shift;
    my $body = shift;
    return { "type" => "Block", "parameters" => @$parameters, "body" => $body };
}


my $program = objCons(cons(importStmt("ast", identifierDeclaration("ast", nil)), cons(comment(" This file makes use of all AST nodes"), cons(defDec("x", nil, nil, objCons(cons(varDec("y", one(lexReq(one(part("Number", nil)))), nil, one(numLit(1))), one(methDec(cons(part("foo", one(identifierDeclaration("arg", one(lexReq(one(part("Action", nil))))))), one(part("bar", one(identifierDeclaration("n", nil))))), one(lexReq(one(part("String", nil)))), nil, cons(assn(dotReq(lexReq(one(part("self", nil))), one(part("y", nil))), dotReq(dotReq(lexReq(one(part("arg", nil))), one(part("apply", nil))), one(part("+", one(lexReq(one(part("n", nil)))))))), one(returnStmt(interpStr(safeStr("y ", charAt, " "), lexReq(one(part("y", nil))), strLit("!")))))))), nil)), one(lexReq(one(part("print", one(dotReq(lexReq(one(part("x", nil))), cons(part("foo", one(block(nil, one(numLit(2))))), one(part("bar", one(numLit(3)))))))))))))), nil);

print Dumper($program);
