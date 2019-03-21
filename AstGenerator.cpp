#include <iostream>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>

#include "AstGenerator.h"
#include "AstGeneratorBase.h"
#include "AstGeneratorJavascriptBlock.h"
#include "Location.h"
#include "parser.h"

using namespace std;
using namespace QmlJS;
using namespace QmlJS::AST;

void AstGenerator::accept(Node *node) { Node::accept(node, this); }

json AstGenerator::operator()(Node *node) {
  accept(node);
  return ast;
}

void AstGenerator::appendItems(const json &items) {
  if (!items.is_array())
    return;

  for (auto &item : items) {
    if (ast.is_object()) {
      if (!ast["children"].is_array())
        ast["children"] = json::array();

      ast["children"].push_back(item);
      continue;
    }

    if (ast.is_array()) {
      ast.push_back(item);
      continue;
    }

    ast = json::array();
    ast.push_back(item);
  }
}

bool AstGenerator::visit(UiImport *node) {
  print("UiImport");

  json item;

  item["kind"] = "Import";
  item["loc"] =
      getLoc(node->importToken, node->fileNameToken, node->versionToken,
             node->asToken, node->importIdToken, node->semicolonToken);

  if (!node->fileName.isNull())
    item["path"] = toString(node->fileName);
  else
    item["identifier"] = toString(node->importUri);

  if (node->versionToken.isValid())
    item["version"] = toString(node->versionToken);

  if (!node->importId.isNull())
    item["as"] = toString(node->importIdToken);

  ast = item;

  return false;
}

bool AstGenerator::visit(UiObjectDefinition *node) {
  print("UiObjectDefinition", toString(node->qualifiedTypeNameId));

  ast["kind"] = "ObjectDefinition";
  ast["loc"] = getLoc(node->firstSourceLocation(), node->lastSourceLocation());
  ast["identifier"] = toString(node->qualifiedTypeNameId);
  ast["children"] = json::array();

  if (node->initializer) {
    AstGenerator gen(doc, level + 1);
    const json item = gen(node->initializer);
    ast["children"] = item;
  }

  return false;
}

bool AstGenerator::visit(UiObjectInitializer *node) {
  print("UiObjectInitializer");

  AstGenerator gen(doc, level + 1);
  appendItems(gen(node->members));

  // json items;

  // for (UiObjectMemberList *it = node->members; it; it = it->next) {
  //   AstGenerator gen(doc, level + 1);
  //   const json item = gen(it);
  //   items.push_back(item);
  // }

  // appendItems(items);

  return false;
}

bool AstGenerator::visit(UiParameterList *list) {
  print("UiParameterList");

  json items;

  for (UiParameterList *it = list; it; it = it->next) {
    json item;

    item["type"] = toString(it->propertyTypeToken);
    item["identifier"] = toString(it->identifierToken);

    items.push_back(item);
  }

  ast = items;

  return false;
}

bool AstGenerator::visit(UiPublicMember *node) {
  print("UiPublicMember", toString(node->identifierToken));

  json item;

  if (node->type == UiPublicMember::Property) {
    item["kind"] = "Property";

    item["loc"] =
        getLoc(node->defaultToken, node->readonlyToken, node->propertyToken,
               node->typeModifierToken, node->typeToken, node->identifierToken,
               node->colonToken, node->semicolonToken);

    if (node->isDefaultMember)
      item["default"] = true;
    else if (node->isReadonlyMember)
      item["readonly"] = true;

    if (!node->typeModifier.isNull())
      item["typeModifier"] = toString(node->typeModifierToken);

    item["type"] = toString(node->typeToken);

    if (node->statement) {
      item["identifier"] = toString(node->identifierToken);

      AstGeneratorJavascriptBlock gen(doc, level + 1);
      item["value"] = gen(node->statement);
    } else if (node->binding) {
      item["identifier"] = toString(node->identifierToken);

      AstGenerator gen(doc, level + 1);
      item["value"] = gen(node->binding);
      // json value = gen(node->binding);
      // item["value"].push_back(value);
    } else
      item["identifier"] = toString(node->identifierToken);
  } else {
    item["kind"] = "Signal";
    item["identifier"] = toString(node->identifierToken);
    item["asBrackets"] = false;

    Location loc;

    if (node->parameters) {
      AstGenerator gen(doc, level + 1);
      item["parameters"] = gen(node->parameters);
      item["asBrackets"] = true;

      // Where is the closing bracket?!
      const Location lastLoc =
          static_cast<Location>(node->parameters->lastSourceLocation());

      const int closingBracketOffset =
          doc->source().indexOf(')', lastLoc.endOffset);

      const lineColumn position = getLineColumn(closingBracketOffset);

      const Location bracketLocation =
          Location(position.column, position.line, closingBracketOffset, 1);

      loc = mergeLocs(node->firstSourceLocation(), bracketLocation);
    } else {
      Location lastLoc = static_cast<Location>(node->identifierToken);

      const int nextCharIndex = getNextPrintableCharIndex(lastLoc.endOffset);
      const QChar nextChar = getCharAt(nextCharIndex);

      if (nextChar == '(') {
        item["asBrackets"] = true;

        const int closingBracketIndex =
            getNextPrintableCharIndex(nextCharIndex);
        const lineColumn position = getLineColumn(closingBracketIndex);

        lastLoc = Location(position.column + 1, position.line,
                           closingBracketIndex + 1, 1);
      }

      loc = mergeLocs(node->firstSourceLocation(), lastLoc);
    }

    item["loc"] = getLoc(loc);
  }

  ast = item;

  return false;
}

bool AstGenerator::visit(UiObjectBinding *node) {
  print("UiObjectBinding");

  json item;

  if (node->hasOnToken) {
    item["kind"] = "ObjectDefinition";
    item["loc"] =
        getLoc(node->firstSourceLocation(), node->lastSourceLocation());
    item["identifier"] = toString(node->qualifiedTypeNameId);
    item["on"] = toString(node->qualifiedId);

    AstGenerator gen(doc, level + 1);
    item["children"] = gen(node->initializer);
  } else {
    item["kind"] = "Attribute";
    item["identifier"] = toString(node->qualifiedId);

    json value;
    value["kind"] = "ObjectDefinition";
    value["loc"] = getLoc(node->qualifiedTypeNameId->firstSourceLocation(),
                          node->initializer->lastSourceLocation());
    value["identifier"] = toString(node->qualifiedTypeNameId);

    AstGenerator gen(doc, level + 1);
    value["children"] = gen(node->initializer);

    item["value"] = value;
  }

  ast = item;

  return false;
}

bool AstGenerator::visit(UiScriptBinding *node) {
  print("UiScriptBinding");

  json item;
  item["kind"] = "Attribute";
  item["identifier"] = toString(node->qualifiedId);

  AstGeneratorJavascriptBlock gen(doc, level + 1);
  item["value"] = gen(node->statement);

  ast = item;

  return false;
}

bool AstGenerator::visit(UiArrayBinding *node) {
  print("UiArrayBinding");

  json item;
  item["kind"] = "ArrayBinding";
  item["loc"] = getLoc(node->lbracketToken, node->rbracketToken);

  item["identifier"] = toString(node->qualifiedId);

  AstGenerator gen(doc, level + 1);
  item["children"] = gen(node->members);

  ast = item;

  return false;
}

bool AstGenerator::visit(UiArrayMemberList *node) {
  print("UiArrayMemberList");

  json items;

  for (UiArrayMemberList *it = node; it; it = it->next) {
    AstGenerator gen(doc, level + 1);
    const json item = gen(it->member);
    items.push_back(item);
  }

  ast = items;

  return false;
}

bool AstGenerator::visit(FunctionDeclaration *node) {
  print("FunctionDeclaration");

  return visit(static_cast<FunctionExpression *>(node));
}

bool AstGenerator::visit(FunctionExpression *node) {
  print("FunctionExpression");

  json item;
  item["kind"] = "Function";
  item["identifier"] = toString(node->identifierToken);

  Location loc = mergeLocs(node->functionToken, node->rbraceToken);
  item["loc"] = getLoc(loc);
  item["body"] = toString(loc);

  ast = item;

  return false;
}

bool AstGenerator::visit(UiHeaderItemList *node) {
  print("UiHeaderItemList");

  ast = json::object();

  const int lastOffset = doc->source().count();
  const lineColumn position = getLineColumn(lastOffset);
  const Location loc =
      Location(0, 1, 0, position.column, position.line, lastOffset);

  ast["kind"] = "Program";
  ast["loc"] = loc.toJson();
  ast["children"] = json::array();

  for (UiHeaderItemList *it = node; it; it = it->next) {
    AstGenerator gen(doc, level + 1);
    const json item = gen(it->headerItem);
    ast["children"].push_back(item);
  }

  return false;
}

bool AstGenerator::visit(UiObjectMemberList *node) {
  print("UiObjectMemberList");

  json items;

  for (UiObjectMemberList *it = node; it; it = it->next) {
    AstGenerator gen(doc, level + 1);
    const json item = gen(it->member);
    items.push_back(item);
  }

  appendItems(items);

  return false;
}