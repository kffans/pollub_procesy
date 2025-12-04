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
        dial::State* state = dial::State_I("unit");
        std::string variableInstruction = "testVariable = (50 + TRUE + FALSE) * 2";
        
        dial::VarInstrInterpret(state, variableInstruction);
        u32 value = dial::GetValue(state, "testVariable").first;
        
        u32 expectedValue = 102;
        State_D(state);
        assert(value == expectedValue);
    }
	void givenConditionInstruction_whenInstructionIsInterpreted_checkIfTrue () {
        dial::State* state = dial::State_I("unit");
        std::string conditionInstruction = "(50 > 40 * 1) AND (\"test\" == \"te\" + \"st\")";
        
        bool value = dial::CondInstrInterpret(state, conditionInstruction);
        
        State_D(state);
        assert(value == true);
    }
	void givenCurrentStatus_whenCheckCurrentStatus_checkIfTrue () {
		dial::State* state = dial::State_I("unit");
        
		bool value = dial::IsCurrentStatus(state, dial::Status::INTERPRET);
		
        State_D(state);
        assert(value == true);
	}
    
    /* integration tests */
    void givenTestFile_whenInterpreted_returnInterpretedText () {
        dial::State* state = dial::State_I("unit");
        
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
	void givenTestFile_whenInterpretedThenSavedAndLoaded_checkIfStateIsTheSameAsBefore () {
        dial::State* state = dial::State_I("unit");
        
        for (u32 i = 0; i < 5; i++) {
            dial::Dialogue_T(state);
            dial::Continuation(state);
        }
        dial::StateSave(state, 0);
		dial::State* loadedState = dial::StateLoad("unit", 0); 
		std::string value = "";
        for (u32 i = 0; i < loadedState->textObjs.size(); i++) {
            value += loadedState->textObjs[i].text;
        }
        
        std::string expectedValue = "Test 1 Test 2 Test 3 Test 5 Test 6 Test 7 ";
        State_D(state);
        State_D(loadedState);
		assert(value == expectedValue);
    }
}

#undef u32
#undef elif

#endif /* test.hpp */

