/********************************************************************
 *             a  A                                                        
 *            AM\/MA                                                         
 *           (MA:MMD                                                         
 *            :: VD
 *           ::  º                                                         
 *          ::                                                              
 *         ::   **      .A$MMMMND   AMMMD     AMMM6    MMMM  MMMM6             
 +       6::Z. TMMM    MMMMMMMMMDA   VMMMD   AMMM6     MMMMMMMMM6            
 *      6M:AMMJMMOD     V     MMMA    VMMMD AMMM6      MMMMMMM6              
 *      ::  TMMTMC         ___MMMM     VMMMMMMM6       MMMM                   
 *     MMM  TMMMTTM,     AMMMMMMMM      VMMMMM6        MMMM                  
 *    :: MM TMMTMMMD    MMMMMMMMMM       MMMMMM        MMMM                   
 *   ::   MMMTTMMM6    MMMMMMMMMMM      AMMMMMMD       MMMM                   
 *  :.     MMMMMM6    MMMM    MMMM     AMMMMMMMMD      MMMM                   
 *         TTMMT      MMMM    MMMM    AMMM6  MMMMD     MMMM                   
 *        TMMMM8       MMMMMMMMMMM   AMMM6    MMMMD    MMMM                   
 *       TMMMMMM$       MMMM6 MMMM  AMMM6      MMMMD   MMMM                   
 *      TMMM MMMM                                                           
 *     TMMM  .MMM                                         
 *     TMM   .MMD       ARBITRARY·······XML········RENDERING                           
 *     TMM    MMA       ====================================                              
 *     TMN    MM                               
 *      MN    ZM                       
 *            MM,
 *
 * 
 *      AUTHORS: Miro Keller
 *      
 *      COPYRIGHT: ©2011 - All Rights Reserved
 *
 *      LICENSE: see License.txt file
 *
 *      WEB: http://axr.vg
 *
 *      THIS CODE AND INFORMATION ARE PROVIDED "AS IS"
 *      WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *      IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *      FITNESS FOR A PARTICULAR PURPOSE.
 *
 ********************************************************************
 *
 *      FILE INFORMATION:
 *      =================
 *      Last changed: 2011/05/02
 *      HSS version: 1.0
 *      Core version: 0.3
 *      Revision: 10
 *
 ********************************************************************/

#include "../hss.h"
#include "HSSValueToken.h"
#include "HSSConstants.h"
#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include "../../axr/AXRDebugging.h"
#include "../../axr/AXRController.h"
#include <boost/pointer_cast.hpp>
#include "HSSExpressions.h"

using namespace AXR;

HSSParser::HSSParser(AXRController * theController)
{
    this->controller = theController;
    this->tokenizer = HSSTokenizer::p(new HSSTokenizer());
    
    this->currentContext.push_back(HSSParserContextRoot);
    //FIXME: will there be a root object? Now defaults to container
    this->currentObjectContext.push(HSSContainer::p(new HSSContainer()));
    std_log1("creating hss parser");
}

//HSSParser::HSSParser(HSSTokenizer::buf_p buffer, unsigned buflen, std::string filename)
//{
//    this->tokenizer = HSSTokenizer::p(new HSSTokenizer(buffer, buflen));
//    this->filename = filename;
//    
//    this->currentContext.push_back(HSSParserContextRoot);
//    //FIXME: will there be a root object? Now defaults to container
//    this->currentObjectContext.push(HSSContainer::p(new HSSContainer()));
//    
//    this->readNextToken();
//}

HSSParser::~HSSParser()
{
    std_log1("destructing hss parser");
    unsigned i;
    for (i=0; i<this->currentObjectContext.size(); i++){
        this->currentObjectContextRemoveLast();
    }
}

void HSSParser::reset()
{
    //clear the tokenizer
    this->tokenizer->reset();
    
    //clear the current object context
    unsigned i;
    for (i=0; i<this->currentObjectContext.size(); i++){
        this->currentObjectContext.pop();
    }
    //clear the current context
    this->currentContext.clear();
    this->filename = std::string();
    this->currentToken.reset();
    
    //initialize the new values
    this->currentContext.push_back(HSSParserContextRoot);
    //FIXME: will there be a root object? Now defaults to container
    this->currentObjectContext.push(HSSContainer::p(new HSSContainer()));
    
}

bool HSSParser::loadFile(std::string filepath)
{
    security_brake_init();
    //get the filename from the path
    this->filename = filepath.substr(filepath.rfind("/", filepath.size())+1);
    
    //open the file for reading
    FILE * hssfile = fopen(filepath.c_str(), "r");
    //read the file into the buffer of the tokenizer
    HSSTokenizer::buf_p hssbuffer = this->tokenizer->getBuffer();
    int len = (int)fread(hssbuffer.get(), 1, AXR_HSS_BUFFER_SIZE, hssfile);
    if (ferror(hssfile)) {
        fclose(hssfile);
        return false;
    }
    //initialize
    this->tokenizer->setBufferLength(len);
    this->tokenizer->readNextChar();
    
    //FIXME: what if the file is longer than the buffer?
    fclose(hssfile);
    
    this->readNextToken();
    
    HSSStatement::p statement;
    
    bool done = false;
    while (!done) {
        std_log1("read statement");
        if(statement){
            statement.reset();
        }
        try {
            statement = this->readNextStatement();
        }
        catch(AXR::HSSUnexpectedTokenException e){
            std::cout << e.toString() << std::endl;
            continue;
        }
        catch(AXR::HSSUnexpectedEndOfSourceException e){
            std::cout << e.toString() << std::endl;
        }
        catch(AXR::HSSUnexpectedObjectTypeException e){
            std::cout << e.toString() << std::endl;
        }
        catch(AXR::HSSExpectedTokenException e){
            std::cout << e.toString() << std::endl;
        }
        catch(AXR::HSSWrongHexLengthException e){
            std::cout << e.toString() << std::endl;
        }
        
        
        if(!statement){
            done = true;
        } else {
//            std::cout << std::endl << "-----------------------------" << std::endl
//            <<  statement->toString() << std::endl << "-----------------------------" << std::endl;
            switch (statement->getType()) {
                case HSSStatementTypeRule:
                {
                    HSSRule::p theRule = boost::static_pointer_cast<HSSRule>(statement);
                    this->controller->rulesAdd(theRule);
                    break; 
                }
                
                default:
                    std_log1("unknown statement");
                    break;
            }
            this->controller->statementsAdd(statement);
        }
        
        security_brake();
    }
    std_log1("reached end of source");
    std_log1("\n\n\n\n");
    
    return true;
}

HSSStatement::p HSSParser::readNextStatement()
{
    HSSStatement::p ret;
    if(this->currentContext.back() == HSSParserContextRoot)
    {
        //the file was empty
        if(this->atEndOfSource())
            return ret;
        
        if(this->currentToken->isA(HSSInstructionSign)){
            this->tokenizer->preferHex = true;
            ret = this->readInstruction();
            this->tokenizer->preferHex = false;
            return ret;
        }
        
        //if the statement starts with an object sign, it is an object definition
        if(this->currentToken->isA(HSSObjectSign)){
            //create an object definition
            ret = this->readObjectDefinition();
            return ret;
        }
        
        //if the statement starts with an identifier, universal selector or combinator it is a rule
        if(this->currentToken->isA(HSSIdentifier)
           || (this->currentToken->isA(HSSSymbol) && VALUE_TOKEN(this->currentToken)->equals(HSSSymbol, "*"))
           || this->isCombinator()){ //FIXME: search for combinators as well
            ret = this->readRule();
            return ret;
        }
        
        if(this->currentToken->isA(HSSBlockComment) || this->currentToken->isA(HSSLineComment)){
            ret = HSSComment::p(new HSSComment(VALUE_TOKEN(this->currentToken)->value));
            this->readNextToken();
            this->skip(HSSWhitespace);
            return ret;
        }
        
        return ret;
    } else {
        std_log1("reading in anything other than root context is not implemented yet");
        return ret;
    }
}

HSSRule::p HSSParser::readRule()
{
    security_brake_init()
    
    //throw error if at end of source
    this->checkForUnexpectedEndOfSource();
    
    //initialize the rule
    HSSSelectorChain::p selectorChain = this->readSelectorChain();
    HSSRule::p ret = HSSRule::p(new HSSRule(selectorChain));
    
    //we're not in a selector anymore
    this->currentContext.pop_back();
    //now we're inside the block
    this->currentContext.push_back(HSSParserContextBlock);
    
    //if the file ends here, fuuuuuuu[...]
    this->checkForUnexpectedEndOfSource();
    
    //we expect a block to open
    this->skipExpected(HSSBlockOpen);
    //skip any whitespace
    this->skip(HSSWhitespace);
    
    //read the inner part of the block
    while (!this->currentToken->isA(HSSBlockClose))
    {
        //std_log1(this->currentToken->toString());
        //if we find an identifier, we must peek forward to see if it is a property name
        if(this->currentToken->isA(HSSIdentifier)){
            if (this->isPropertyDefinition()){
                HSSPropertyDefinition::p propertyDefinition = this->readPropertyDefinition();
                ret->propertiesAdd(propertyDefinition);
            } else {
                //recursive omg!
                ret->childrenAdd(this->readRule());
            }
        } else {
            throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        }
        
        security_brake()
    }
    security_brake_reset()
    
    //we're out of the block, read next token
    this->readNextToken();
    //leave the block context
    this->currentContext.pop_back();
    if(!this->atEndOfSource()){
        //ignore all the whitespace after the block
        this->skip(HSSWhitespace);
    }
    
    return ret;
}

HSSSelectorChain::p HSSParser::readSelectorChain()
{
    security_brake_init();
    
    HSSSelectorChain::p ret = HSSSelectorChain::p(new HSSSelectorChain());
    
    //first we need to look at the selector chain
    //set the appropriate context
    this->currentContext.push_back(HSSParserContextSelectorChain);
    //parse the selector chain until we find the block
    while (this->currentToken && !this->currentToken->isA(HSSBlockOpen)) {
        std_log3(this->currentToken->toString());
        
        //if it's an identifier, it's a simple selector
        if (this->currentToken->isA(HSSIdentifier)){
            ret->add(this->readSelector());
            //adds only if needed
            HSSCombinator::p childrenCombinator(this->readChildrenCombinatorOrSkip());
            if(childrenCombinator){
                ret->add(childrenCombinator);
            }
            
            //a symbol, probably a combinator
        } else if (this->currentToken->isA(HSSSymbol)) {
            const char currentTokenValue = *(VALUE_TOKEN(this->currentToken)->value).c_str();
            switch (currentTokenValue) {
                case '=':
                case '-':
                case '+':
                case '>': //FIXME: special handling for text selection combinators?
                    ret->add(this->readSymbolCombinator());
                    break;
                case '*':
                    ret->add(HSSUniversalSelector::p(new HSSUniversalSelector()));
                    this->readNextToken();
                    //adds only if needed
                    ret->add(this->readChildrenCombinatorOrSkip());
                    
                    break;
                case '.':
                    //we need to check if it is really a combinator or just a syntax error
                    if(   VALUE_TOKEN(this->currentToken)->value == ".."
                       || VALUE_TOKEN(this->currentToken)->value == "..."  ){
                        ret->add(this->readSymbolCombinator());
                        break;
                    }
                    //huh? we didn't expect any other symbol
                default:
                    throw HSSUnexpectedTokenException(HSSSymbol, std::string(1,currentTokenValue), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
            }
            //we didn't expect any other type of token
        } else {
            throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        }
        
        security_brake();
    }
    security_brake_reset();
    
    return ret;
}

bool HSSParser::isCombinator()
{
    return this->isCombinator(this->currentToken);
}

bool HSSParser::isCombinator(HSSToken::p token)
{
    //are we in a context that accepts combinators?
    HSSParserContext context = this->currentContext.back();
    if (context == HSSParserContextExpression) {
        return false;
    }
    //all combinators are symbols
    if(token->isA(HSSSymbol) ){
        const char currentTokenChar = *(VALUE_TOKEN(token).get()->value).c_str();
        switch (currentTokenChar) {
            case '=':
            case '-':
            case '+':
            case '.':
            case '>':
                return true;
                //fixme
                
            default:
                return false;
        }
    } else if ( token->isA(HSSWhitespace) ){
        return this->isChildrenCombinator();
    }
    return false;
}

//this function assumes currentToken to be whitespace
bool HSSParser::isChildrenCombinator()
{
    std_log4("----- peeking ------ ");
    //if the next token is anything other than a combinator or an open block the whitespace means children combinator
    HSSToken::p peekToken = this->tokenizer->peekNextToken();
    std_log4(peekToken->toString());
    bool ret = !this->isCombinator(peekToken) && !peekToken->isA(HSSBlockOpen);
    this->tokenizer->resetPeek();
    std_log4("----- finished peeking ------ ");
    return ret;
}

//this function assumes currentToken to be an identifier
bool HSSParser::isPropertyDefinition()
{
    bool ret = true;
    
    std_log4("----- peeking ------ ");
    HSSToken::p peekToken = this->tokenizer->peekNextToken();
    std_log4(peekToken->toString());
    //if the next token is a colon, it is either a property definition or a filter
    if(peekToken->isA(HSSColon)){
        //we'll peek until we find a end of statement, a closing block or an opening one
        peekToken = this->tokenizer->peekNextToken();
        while(! peekToken->isA(HSSEndOfStatement) && !peekToken->isA(HSSBlockClose) && !peekToken->isA(HSSBlockOpen))
        {
            std_log1(peekToken->toString());
            peekToken = this->tokenizer->peekNextToken();
            this->checkForUnexpectedEndOfSource();
        }
        //if we find an opening block, we're dealing with a selector
        if(peekToken->isA(HSSBlockOpen)){
            ret = false;
        }
        
//        peekToken = this->tokenizer->peekNextToken();
//        //now, if we're dealing with an identifier it may be a filter
//        if(peekToken->isA(HSSIdentifier)) {
//            //if it is, after the identifier can only come a parenthesis open or a whitespace
//            peekToken = this->tokenizer->peekNextToken();
//            if(! peekToken->isA(HSSParenthesisOpen) && ! peekToken->isA(HSSWhitespace) )
//            {
//                //we still don't know... continue peeking
//                peekToken = this->tokenizer->peekNextToken();
//                
//            }
//        }
    } else {
        //no colon, no property definiton
        ret = false;
    }
    std_log4("----- finished peeking ------ ");
    this->tokenizer->resetPeek();
    return ret;
}

HSSCombinator::p HSSParser::readChildrenCombinatorOrSkip()
{
    HSSCombinator::p ret;
    //are we dealing with whitespace?
    if(this->currentToken->isA(HSSWhitespace)){
        if(this->isChildrenCombinator()){
            HSSCombinator::p newCombinator = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypeChildren));
            this->readNextToken();
            return newCombinator;
        } else {
            //alright, ignore it
            this->skip(HSSWhitespace);
            return ret;
        }
    } else {
        //done, nothing to see here, move along...
        return ret;
    }
}

//this expects the current token to be a symbol
HSSCombinator::p HSSParser::readSymbolCombinator()
{
    //FIXME: check the context
    HSSCombinator::p ret;
    const char currentTokenChar = *(VALUE_TOKEN(this->currentToken)->value).c_str();
    switch (currentTokenChar) {
        case '=':
            ret = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypeSiblings));
            break;
        case '-':
            ret = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypePreviousSiblings));
            break;
        case '+':
            ret = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypeNextSiblings));
            break;
        case '.':
            if(VALUE_TOKEN(this->currentToken)->value == ".."){
                ret = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypeDescendants));
            } else if (VALUE_TOKEN(this->currentToken)->value == "..."){
                ret = HSSCombinator::p(new HSSCombinator(HSSCombinatorTypeAllDescendants));
            }
            
            break;
        default:
            throw HSSUnexpectedTokenException(HSSSymbol, std::string(1, currentTokenChar), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
            return ret;
    }
    
    this->readNextToken();
    this->skip(HSSWhitespace);
    return ret;
}

//this assumes that the currentToken is an identifier
HSSSelector::p HSSParser::readSelector()
{
    std::string theValue = VALUE_TOKEN(this->currentToken)->value;
    HSSSelector::p ret = HSSSelector::p(new HSSSelector(theValue));
    this->readNextToken();
    return ret;
}


//this assumes currentToken is an object sign
HSSObjectDefinition::p HSSParser::readObjectDefinition()
{
    HSSObjectDefinition::p ret;
    std::string objtype;
    HSSObject::p obj;
    
    //yeah, yeah, we know the @ already
    this->skipExpected(HSSObjectSign);
    //end of file would be fatal
    this->checkForUnexpectedEndOfSource();
    
    //store the current context for later use
    HSSParserContext outerContext = this->currentContext.back();
    //set the appropriate context
    this->currentContext.push_back(HSSParserContextObjectDefinition);
    
    //first we need to know what type of object it is
    if (this->currentToken->isA(HSSWhitespace)) {
        //damn, we'll have to derive that from the context
        if (outerContext == HSSParserContextRoot){
            objtype = "container";
        } else {
            //FIXME
            std_log1("deriving object types from context is only supported in root context yet");
            objtype = "object";
        }
    } else if(this->currentToken->isA(HSSIdentifier)){
        //alright, we've got a type, look it up
        objtype = VALUE_TOKEN(this->currentToken)->value;
        this->readNextToken();
        this->checkForUnexpectedEndOfSource();
    } else {
        throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        return ret;
    }
    
    //try to create an object of that type
    try {
        obj = HSSObject::newObjectWithType(objtype);
    } catch (HSSUnknownObjectTypeException e) {
        throw HSSUnexpectedObjectTypeException(e.type, this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
    }
    
    
    //get the name of the object
    if (this->currentToken->isA(HSSWhitespace)) {
        this->skip(HSSWhitespace);
    }
    if (this->currentToken->isA(HSSIdentifier)) {
        obj->setName(VALUE_TOKEN(this->currentToken)->value);
        this->readNextToken();
    } else if (this->currentToken->isA(HSSBlockOpen)){
        //it is the opening curly brace, therefore an annonymous object:
        //do nothing
    } else {
        throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
    }
    
    ret = HSSObjectDefinition::p(new HSSObjectDefinition(obj));
    this->skip(HSSWhitespace);
    this->skipExpected(HSSBlockOpen);
    this->skip(HSSWhitespace);
    
    //now we're inside the block
    this->currentContext.push_back(HSSParserContextBlock);
    
    //if the file ends here, fuuuuuuu[...]
    this->checkForUnexpectedEndOfSource();
    
    //read the inner part of the block
    while (!this->currentToken->isA(HSSBlockClose)){
        ret->propertiesAdd(this->readPropertyDefinition());
    }
    
    //we're out of the block, we expect a closing brace
    this->skipExpected(HSSBlockClose);
    //leave the block context
    this->currentContext.pop_back();
    //leave the object definition context
    this->currentContext.pop_back();
    if(!this->atEndOfSource()){
        //ignore all the whitespace after the block
        this->skip(HSSWhitespace);
    }
    
    return ret;
}

HSSPropertyDefinition::p HSSParser::readPropertyDefinition()
{
    std::string propertyName;
    
    //end of source is no good
    this->checkForUnexpectedEndOfSource();
    
    HSSPropertyDefinition::p ret;
    
    if (this->currentToken->isA(HSSIdentifier)){
        propertyName = VALUE_TOKEN(this->currentToken)->value;
        ret = HSSPropertyDefinition::p(new HSSPropertyDefinition(propertyName));
        this->readNextToken();
        //now must come a colon
        this->skipExpected(HSSColon);
        //we don't give a f$%# about whitespace
        this->skip(HSSWhitespace);
        //now comes either an object definition, a literal value or an expression
        //object
        if (this->currentToken->isA(HSSObjectSign)){
            ret->setValue(this->readObjectDefinition());
            //this->readNextToken();
            
        } else if (this->currentToken->isA(HSSSingleQuoteString) || this->currentToken->isA(HSSDoubleQuoteString)){
            ret->setValue(HSSStringConstant::p(new HSSStringConstant(VALUE_TOKEN(this->currentToken)->value)));
            this->checkForUnexpectedEndOfSource();
            this->readNextToken();
            
        //number literal
        } else if (this->currentToken->isA(HSSNumber) || this->currentToken->isA(HSSPercentageNumber) || this->currentToken->isA(HSSParenthesisOpen)){
            
            //FIXME: parse the number and see if it is an int or a float
            HSSParserNode::p exp = this->readExpression();
            ret->setValue(exp);
            
        } else if (this->currentToken->isA(HSSIdentifier)){
            //this is either a keyword or an object name
            //check if it is a keyword
            std::string valuestr = VALUE_TOKEN(this->currentToken)->value;
            if(this->currentObjectContext.top()->isKeyword(valuestr, propertyName)){
                ret->setValue(HSSKeywordConstant::p(new HSSKeywordConstant(valuestr))); 
            } else {
                //FIXME
                ret->setValue(HSSObjectNameConstant::p(new HSSObjectNameConstant(valuestr)));
            }
            this->readNextToken();
        } else if (this->currentToken->isA(HSSInstructionSign)){
            this->tokenizer->preferHex = true;
            ret->setValue(this->readInstruction());
            this->tokenizer->preferHex = false;
            
        } else {
            throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        }
        
        this->skip(HSSWhitespace);
        //expect a semicolon or the closing brace
        if(this->currentToken->isA(HSSEndOfStatement)){
            this->readNextToken();
            this->skip(HSSWhitespace);
            
        } else if (this->currentToken->isA(HSSBlockClose)){
            //alright, this is the end of the property definition
            std_log3("end of property definition");
        } else {
            throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        }
        
        return ret;
    } else {
        throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        this->readNextToken();
    }
    return ret;
}


HSSInstruction::p HSSParser::readInstruction()
{
    HSSInstruction::p ret;
    std::string currentval;
    
    this->skipExpected(HSSInstructionSign);
    this->checkForUnexpectedEndOfSource();
    //we are looking at
    //if it starts with a number, we are looking at a color instruction
    if (this->currentToken->isA(HSSHexNumber)){
        currentval = VALUE_TOKEN(this->currentToken)->value;
        switch (currentval.length()) {
            //1 digit grayscale
            case 1:
            //2 digit grayscale
            case 2:
            //rgb
            case 3:
            //rgba
            case 4:
            //rrggbb
            case 6:
            //rrggbbaa
            case 8:
                ret = HSSInstruction::p(new HSSInstruction(HSSColorInstruction, currentval));
                this->readNextToken();
                break;
                
            default:
                throw HSSWrongHexLengthException(currentval.length(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
                return ret;
        }
                
    } else if (this->currentToken->isA(HSSIdentifier)){
        currentval = VALUE_TOKEN(this->currentToken)->value;
        if (currentval == "new"){
            ret = HSSInstruction::p(new HSSInstruction(HSSNewInstruction));
        } else if (currentval == "ensure") {
            ret = HSSInstruction::p(new HSSInstruction(HSSEnsureInstruction));
        } else if (currentval == "import") {
            ret = HSSInstruction::p(new HSSInstruction(HSSImportInstruction));
        } else {
            throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
            return ret;
        }
        this->readNextToken();
        
    } else {
        throw HSSUnexpectedTokenException(this->currentToken->getType(), this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
        return ret;
    }
    
    this->checkForUnexpectedEndOfSource();
    this->skip(HSSWhitespace);
    return ret;
}

HSSParserNode::p HSSParser::readExpression()
{
    return this->readAdditiveExpression();
}

HSSParserNode::p HSSParser::readAdditiveExpression()
{
    security_brake_init();
    
    this->checkForUnexpectedEndOfSource();
    HSSParserNode::p left = this->readMultiplicativeExpression();
    while (!this->atEndOfSource() && this->currentToken->isA(HSSSymbol)) {
        const char currentTokenChar = *(VALUE_TOKEN(this->currentToken)->value).c_str();
        switch (currentTokenChar) {
            case '+':
            {
                this->readNextToken();
                this->skip(HSSWhitespace);
                left = HSSSum::p(new HSSSum(left, this->readMultiplicativeExpression()));
                break;
            }
                
            case '-':
            {
                this->readNextToken();
                this->skip(HSSWhitespace);
                left = HSSSubtraction::p(new HSSSubtraction(left, this->readMultiplicativeExpression()));
                break;
            }
                
            default:
                return left;
                break;
        }
        
        security_brake();
    }
    
    return left;
}

HSSParserNode::p HSSParser::readMultiplicativeExpression()
{
    security_brake_init();
    
    this->checkForUnexpectedEndOfSource();
    HSSParserNode::p left = this->readBaseExpression();
    while (!this->atEndOfSource() && this->currentToken->isA(HSSSymbol)) {
        
        const char currentTokenChar = *(VALUE_TOKEN(this->currentToken)->value).c_str();
        switch (currentTokenChar) {
            case '*':
            {
                this->readNextToken();
                this->skip(HSSWhitespace);
                left = HSSMultiplication::p(new HSSMultiplication(left, this->readBaseExpression()));
                break;
            }
                
            case '/':
            {
                this->readNextToken();
                this->skip(HSSWhitespace);
                left = HSSDivision::p(new HSSDivision(left, this->readBaseExpression()));
                break;
            }
                
            default:
                return left;
                break;
        }
        
        security_brake();
    }
    
    return left;
}

HSSParserNode::p HSSParser::readBaseExpression()
{
    this->checkForUnexpectedEndOfSource();
    HSSParserNode::p left;
    
    switch (this->currentToken->getType()) {
        case HSSNumber:
        {
            left = HSSNumberConstant::p(new HSSNumberConstant(strtold(VALUE_TOKEN(this->currentToken)->value.c_str(), NULL)));
            this->readNextToken();
            this->skip(HSSWhitespace);
            break;
        }
        
        case HSSPercentageNumber:
        {
            left = HSSPercentageConstant::p(new HSSPercentageConstant(strtold(VALUE_TOKEN(this->currentToken)->value.c_str(), NULL)));
            this->readNextToken();
            this->skip(HSSWhitespace);
            break;
        }
            
        case HSSParenthesisOpen:
        {
            this->readNextToken();
            this->skip(HSSWhitespace);
            left = this->readExpression();
            this->skipExpected(HSSParenthesisClose);
            this->skip(HSSWhitespace);
            break;
        }
        
        default:
            throw "Unknown token type while parsing base expression";
            break;
    }
    
    return left;
}

void HSSParser::readNextToken()
{
    //read next one
    this->currentToken = this->tokenizer->readNextToken();
}

bool HSSParser::atEndOfSource()
{
    if(!this->currentToken){
        return true;
    } else {
        return false;
    }
}

void HSSParser::checkForUnexpectedEndOfSource()
{
    if (this->atEndOfSource()) {
        throw HSSUnexpectedEndOfSourceException(this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
    }
}

void HSSParser::skipExpected(HSSTokenType type)
{
    this->checkForUnexpectedEndOfSource();
    if (!this->currentToken->isA(type)) {
        throw HSSExpectedTokenException(type, this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
    }
    this->readNextToken();
}

void HSSParser::skipExpected(HSSTokenType type, std::string value)
{
    this->checkForUnexpectedEndOfSource();
    //FIXME: I'm not sure if this works as expected
    HSSValueToken::p currentToken = HSSValueToken::p(VALUE_TOKEN(this->currentToken));
    if (!currentToken->equals(type, value)) {
        throw HSSExpectedTokenException(type, value, this->filename, this->tokenizer->currentLine, this->tokenizer->currentColumn);
    }
    this->readNextToken();
}

void HSSParser::skip(HSSTokenType type)
{
    if(this->currentToken->isA(type)){
        this->readNextToken();
    }
}

void HSSParser::currentObjectContextRemoveLast()
{
    this->currentObjectContext.pop();
}

