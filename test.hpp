#ifndef TEST_HPP
#define TEST_HPP

#define u32 unsigned int
#define elif else if


#include <cassert>
#include "dial.hpp"

namespace test {
    /* unit tests */
	void givenUnformattedText_whenRemovedWhitespace_returnCleanText () {
        std::string unformattedText = "\t  testing  test\n\rt\text  123  \n\t ";
        
        std::string value = dial::RemoveWhitespace(unformattedText);
        
        std::string expectedValue = "testing test text 123 ";
        assert(value == expectedValue);
    }
    void givenVariableInstruction_whenInstructionIsInterpreted_checkIfVariableChanged () {
        dial::State* state = dial::State_I("unit_test");
        std::string variableInstruction = "testVariable = (50 + TRUE + FALSE) * 2";
        
        dial::VarInstrInterpret(state, variableInstruction);
        u32 value = dial::GetValue(state, "testVariable").first;
        
        u32 expectedValue = 102;
        State_D(state);
        assert(value == expectedValue);
    }
    
    /* integration tests */
    void givenTestFile_whenInterpreted_returnInterpretedText () {
        dial::State* state = dial::State_I("unit_test");
        
        for (u32 i = 0; i < 10; i++) {
            dial::Dialogue_T(state);
            dial::Continuation(state);
        }
        std::string value = "";
        for (u32 i = 0; i < state->textObjs.size(); i++) {
            value += state->textObjs[i].text;
        }
        
        std::string expectedValue = "Test 1 Test 2 Test 3 Test 4 Test 5 Test 6 Test 7 Test 8 Test 9 ";
        State_D(state);
        assert(value == expectedValue);
    }
}

#undef u32
#undef elif

#endif /* test.hpp */

