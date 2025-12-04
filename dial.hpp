#ifndef DIAL_HPP
#define DIAL_HPP

#define u32 unsigned int
#define elif else if

// NAMING CONVENTIONS:
// functions might end with:
// _I   init, initializing function or struct/class
// _D   destroy, deallocate members of struct/class
// _P   pointer allocated, destroy it later
// _A   array allocated, destroy it later
// _T   tick function
//
// variables might end with:
// _s   size (of something)
// _i   index
// _c   count (of something)
// _n   name (e.g filename)
// _b   buffer, temporary
//
// bools start with:
// is, has, was
//
// functions which return bool start with:
// Is, Has, Was
//
// shortened names:
// Instruction = instr
// Conditional = cond
// Position    = pos
//

namespace dial {
    using std::string;
    using std::map;
    using std::vector;
    using std::set;
    
    /* Constants */
    const u32 DIAL_DEFAULT_TEXT_WIDTH = 40;
    const u32 DIAL_JUMP_LOOP_LIMIT = 100;

    enum class TextType {
        NONE,
        NORMAL,
        CHOICE_NORMAL,
        CHOICE_SELECTED,
        CHOICE_ACCENTED,
        CHOICE_CHANCE
    };

    struct TextObject {
        string actor_n; /* @TODO does it need to be here? */
        string text;
        TextType type;
    };

    struct Pos {
        u32 text_i;
        int condNestingDepth;
    };

    struct ChoiceObject {
        string instrText;
        string displayText;
        TextType type;
        map<string,string> accentedOptions;
        Pos jumpPos;
    };

    enum class Status {
        NONE,
        WAIT_FOR_CONTINUATION,
        WAIT_FOR_CHOICE,
        INTERPRET,
        FATAL_ERROR,
        FINISHED
    };

    enum class Flags {
        AUTO_LOAD = 1
    };

    struct State {
        char* text;
        u32 text_s;
        u32 textWidth;
        string displayText;
        string actor_n;
        vector<TextObject> textObjs;
        Status status;
        Flags flags;
        Pos currentPos;
        map<u32, Pos> jumpBasePos;
        vector<std::pair<int,Pos>> jumpHistory;
        vector<std::pair<string,string>> persCond;
        vector<bool> condElse;
        map<string,std::pair<int,string>> localVars;
        vector<ChoiceObject> choices;
        set<string>::iterator currentAccent;
        set<string> possibleAccents;
        map<u32, bool> hasOneUseChoiceRecurred;
        map<u32, int> condRepeat_c; 
        u32 seedRandom;
        map<string, std::pair<int,string>> globalVarsCopy;
        vector<string> saveData;
    };


    map<string, std::pair<int,string>> Vars; /* global variables */


    State* State_I (string file_n);
    void   State_D (State*& state);
    void   StateSave (State* state, int save_i);
    State* StateLoad (string file_n, int save_i);
    void   LoadJumpBases (State* state);

    void   SeekUntil (char* txt, u32& t_i, string endingChars);
    void   SeekEndOfConditional (char* txt, u32& t_i);
    void   SeekEndOfChoiceRange (char* txt, u32& t_i);
    void   SeekEndOfStatement (char* txt, u32& t_i, string endingChar);
    string ScanTextUntil (char* txt, u32& t_i, string endingChars);
    bool   IsItConditionalChoice (char* txt, u32 t_i);

    void   ShowRefreshedText (State* state);
    
    void Dialogue_T (State* state);


    int stringToInt (string varText, bool& hasSucceeded) { /* tries to convert string to int and shows whether the conversion succeeded */
        char* pos;
        int converted = strtol(varText.c_str(), &pos, 10);
        hasSucceeded = !(*pos);
        if (hasSucceeded) {
            return converted;
        }
        else {
            return 0;
        }
    }

// // // ERROR-HANDLING FUNCTIONS // // //

    string GetCurrentTextFilePos (char* txt, u32 target_i) {
        int line_c = 1; /* current line */
        int col_c = 0;  /* current column */
        for (u32 i = 0; i != target_i; i++) {
            if (txt[i] == '\n') {
                line_c++;
                col_c = 0;
            }
            col_c++;
        }
        return "(Line:" + std::to_string(line_c) + ", Col:" + std::to_string(col_c) + ")";
    }

    void Error (string errText, char* txt, u32 t_i) {
        #ifdef DIAL_DEBUG
        std::cerr<<"\n:ERROR: "<<GetCurrentTextFilePos(txt, t_i)<<" "<<errText<<"\n\n";
        #endif
        #ifndef DIAL_DEBUG
        /* @TODO if not DIAL_DEBUG, then could save the errors to a log file */
        #endif
    }

    void Error (string errText) {
        #ifdef DIAL_DEBUG
        std::cerr<<"\n:ERROR: "<<errText<<"\n\n";
        #endif
        #ifndef DIAL_DEBUG
        /* @TODO if not DIAL_DEBUG, then could save the errors to a log file */
        #endif
    }

    #ifdef DIAL_DEBUG
    //vector<State> backtrack;
    //map<string, std::pair<int,string>> backtrackVars;
    bool isBacktrackLocked = false;
    void SaveBacktrackState (State* state) {
        if (state == nullptr) { return; }

        /*
        State backtrackStateSave;

        backtrackStateSave.displayText = "";
        backtrackStateSave.currentPos = state->currentPos;
        backtrackStateSave.condElse = state->condElse;
        backtrackStateSave.hasOneUseChoiceRecurred = state->hasOneUseChoiceRecurred;
        backtrackStateSave.backtrackVars = Vars;

        state->backtrack.push_back(backtrackStateSave);
        */
    }

    void LoadBacktrackState (State* state) {
        if (state == nullptr) { return; }
        
        /*
        if (state->backtrack.size() > 1) {
            state->backtrack.pop_back();
            State backtrack = state->backtrack.back();

            state->displayText = "";
            state->status = Status::INTERPRET;
            state->currentPos = backtrack.currentPos;
            state->condElse = backtrack.condElse;
            state->hasOneUseChoiceRecurred = backtrack.hasOneUseChoiceRecurred;
            Vars = backtrack.backtrackVars;

            state->backtrack.pop_back();
        }
        */
    }
    #endif

    bool HasDetectedCriticalErrors (State* state) {
        if (state == nullptr) { return false; }

        char* txt = state->text;
        u32& t_i = state->currentPos.text_i;
        bool result = false;

        struct SpecChar {
            bool isStatementOpened = false;
            int count = 0;
            int lockedOut_c = 0;
            u32 pos_i = 0;
        };

        map<char, SpecChar> chars;
        while (t_i != state->text_s) {
            switch (txt[t_i]) {
                case '#': { /* @&{}| */
                    chars['#'].isStatementOpened = !chars['#'].isStatementOpened;
                    chars['#'].count++;
                    chars['#'].pos_i = t_i;
                    break;
                }
                case '@': { /* #&{}| */
                    chars['@'].isStatementOpened = !chars['@'].isStatementOpened;
                    chars['@'].count++;
                    chars['@'].pos_i = t_i;
                    break;
                }
                case '[': {
                    chars['['].isStatementOpened = true;
                    chars['['].pos_i = t_i;
                    break;
                }
                case ']': chars['['].isStatementOpened = false; break;
                case '{': {
                    chars['{'].isStatementOpened = true;
                    chars['{'].count++;
                    chars['{'].pos_i = t_i;
                    break;
                }
                case '}': {
                    chars['{'].isStatementOpened = false;
                    chars['}'].count++;
                    chars['}'].pos_i = t_i;
                    break;
                }
                case '&': { /* #&| */
                    chars['&'].isStatementOpened = !chars['&'].isStatementOpened;
                    chars['&'].count++;
                    chars['&'].pos_i = t_i;
                    break;
                }
                case '|': {
                    if (txt[t_i + 1] == '~') {
                        chars['~'].count++;
                        goto EndOfCounting;
                    }
                    elif (txt[t_i + 1] == '|') {
                        chars['|'].count++;
                        t_i++;
                    }
                    break;
                }
                default: break;
            }
            t_i++;
        }
        EndOfCounting:
        t_i = 0;

        /* @TODO positions in detected errors, where possible */
        if (chars['~'].count == 0)                     { Error("The text doesn't have the file ending symbol '|~'."); result = true; }
        if (chars['&'].isStatementOpened)              { Error("One of the conditional instructions doesn't have a corresponding pair '&'."); result = true; }
        elif (chars['&'].count > chars['|'].count * 2) { Error("There's not enough conditional scope ending '||' symbols"); }
        elif (chars['&'].count < chars['|'].count * 2) { Error("There's too many conditional scope ending '||' symbols"); }
        if (chars['#'].isStatementOpened)              { Error("One of the variable instructions doesn't have a corresponding pair '#'."); result = true; }
        if (chars['@'].isStatementOpened)              { Error("One of the special instructions doesn't have a corresponding pair '@'."); result = true; }
        if (chars['['].isStatementOpened)              { Error("There's an unclosed jump point or jump base and is missing a ']' symbol."); result = true; }
        if (chars['{'].isStatementOpened)              { Error("There's an unclosed choice range and is missing a '}' symbol."); result = true; }
        if (chars['{'].count > chars['}'].count)       { Error("One of choice ranges or choices is not closed with it's corresponding pair '}'."); }
        if (chars['{'].count < chars['}'].count)       { Error("One of choice ranges or choices is not opened with it's corresponding pair '{'."); }
        return result;
    }


    void ShowVars (State* state) { /* shows all used variables */
        std::cout<<"\n-------LOCAL--------+\n";
        if (state != nullptr) {
            for (map<string, std::pair<int,string>>::iterator it = state->localVars.begin(); it != state->localVars.end(); it++) {
                if ((it->second).second == "") {
                    std::cout<<std::setw(18)<<it->first<<" = "<<(it->second).first<<'\n';
                }
                else {
                    std::cout<<std::setw(18)<<it->first<<" = \""<<(it->second).second<<"\"\n";
                }
            }
        }
        std::cout<<"\n-------GLOBAL-------+\n";
        for (map<string, std::pair<int,string>>::iterator it = Vars.begin(); it != Vars.end(); it++) {
            if ((it->second).second == "") {
                std::cout<<std::setw(18)<<it->first<<" = "<<(it->second).first<<'\n';
            }
            else {
                std::cout<<std::setw(18)<<it->first<<" = \""<<(it->second).second<<"\"\n";
            }
        }
        std::cout<<"--------------------+\n";
    }
    
    void SaveVarDiff (State* state) {
        if (state == nullptr) { Error("Couldn't save a difference of variables at the state was deleted."); return; }
        map<string, std::pair<int,string>> diffVars;
        for (auto& pair : state->globalVarsCopy) {
            if (!Vars.count(pair.first)) {
                diffVars[pair.first] = pair.second;
            }
        }
        for (auto& pair : Vars) {
            if (!(state->globalVarsCopy).count(pair.first)){
                diffVars[pair.first] = pair.second;
            }
        }
        for (auto& pair : state->globalVarsCopy) {
            auto it = Vars.find(pair.first);
            if (it != Vars.end() && it->second != pair.second) {
                diffVars[pair.first] = pair.second;
            }
        }
        for (auto& pair : diffVars) {
            if ((pair.second).second != "") {                        
                state->saveData.push_back("v:" + pair.first + " = \"" + (pair.second).second + "\"");
            }
            else {
                state->saveData.push_back("v:" + pair.first + " = " +  std::to_string((pair.second).first));   
            }
        }
        for (auto& pair : Vars) {
            state->globalVarsCopy[pair.first] = pair.second;
        }
    }

    std::pair<int,string>& GetVar (State* state, string key, bool isNegated) {
        if (state == nullptr) { Error("Local variable was not available as the state was deleted; returning a global variable."); return Vars[key]; }
        bool isNegative = false;
		if (key.length() != 0 && key[0] == '-') {
			isNegative = true;
			key = key.substr(1, key.length() - 1);
		}
		if (key.length() != 0) {
            if (key == "REPEAT")      { Vars[key].first =  state->condRepeat_c[state->currentPos.text_i]; Vars[key].second = ""; }
            if (key == "ONCE")        { Vars[key].first = !state->condRepeat_c[state->currentPos.text_i]; Vars[key].second = ""; }
            if (key == "TRUE")        { Vars[key].first = 1; Vars[key].second = ""; }
            if (key == "FALSE")       { Vars[key].first = 0; Vars[key].second = ""; }
            if (key == "RANDOM")      { Vars[key].first = (rand() % 100) + 1; Vars[key].second = ""; } /* generates number from 1 to 100*/
            
            if (std::isupper(key[0])) {
				if (isNegative) {
					Vars[key].first = (-1) * Vars[key].first;
				}
				if (isNegated) {
					Vars[key].first = (int)(!Vars[key].first);					
				}
				return Vars[key];
			}
            else {
				if (isNegative) {
					(state->localVars[key]).first = (-1) * (state->localVars[key]).first;
				}
				if (isNegated) {
					(state->localVars[key]).first = (int)(!(state->localVars[key]).first);					
				}
				return state->localVars[key]; 
			}
        }
        else {
            Error("Could not get a variable as its length is 0.", state->text, state->currentPos.text_i);
        }
        return state->localVars[key];
    }

    void LoadJumpBases (State* state) {
        if (state == nullptr) { return; }

        char* txt = state->text;
        u32& t_i = state->currentPos.text_i;
        int condNestingDepth_b = 0;

        while (true) {
            switch (txt[t_i]) {
                case '#': { SeekEndOfStatement(txt, t_i, "#"); break; }
                case '@': { SeekEndOfStatement(txt, t_i, "@"); break; }
                case '{': { 
                    t_i++;
                    u32 potentialStartOfChoiceRangePos_i = t_i;
                    SeekUntil(txt, t_i, "{}");
                    if (txt[t_i] == '{') {
                        t_i = potentialStartOfChoiceRangePos_i;
                    }
                    else {
                        t_i++;
                    }
                    break;
                }
                case '[': {
                    if (txt[t_i + 1] == '[') { /* [[...]] */
                        t_i++;
                        u32 beginningOfJumpBaseNumber_i = t_i;
                        string jumpBaseNumberText = ScanTextUntil(txt, t_i, "-_ ]"); /* examples: [[20-desc.]] , [[7_the scene]] , [[6 plot branch]] , [[38]] */
                        bool hasSucceeded; int jumpBaseNumber = stringToInt(jumpBaseNumberText, hasSucceeded);
                        if (hasSucceeded) {
                            t_i = beginningOfJumpBaseNumber_i;
                            SeekUntil(txt, t_i, "]");
                            while (txt[t_i] == ']') { /* ]]*... */
                                t_i++;
                            }

                            Pos jumpBase_b;
                            jumpBase_b.text_i = t_i;
                            jumpBase_b.condNestingDepth = condNestingDepth_b;
                            state->jumpBasePos[jumpBaseNumber] = jumpBase_b;
                        }
                        else {
                            Error("Following jump base number could not be interpreted: " + jumpBaseNumberText, txt, t_i);
                        }
                    }
                    else { /* [...] */
                        t_i++;
                    }
                    break;
                }
                case '&': {
                    condNestingDepth_b++;
                    SeekEndOfStatement(txt, t_i, "&");
                    break;
                }
                case '|': {
                    if (txt[t_i + 1] == '~') { /* |~ */
                        t_i = 0;
                        return;
                    }
                    elif (txt[t_i + 1] == '|') { /* || */
                        condNestingDepth_b--;
                        if (condNestingDepth_b < 0) {
                            Error("Conditional nesting depth is below zero. There are stray '||' symbols.", txt, t_i);
                            condNestingDepth_b = 0;
                        }
                        t_i += 2;
                    }
                    else { /* | */
                        t_i++;
                    }
                    break;
                }
                default: t_i++; break;
            }
        }
    }


    bool IsWhitespace (char character) {
        return (character == ' ' || character == '\t' || character == '\n' || character == '\r');
    }

    string RemoveWhitespace (string dirtyText) {
        string cleanText = "";
        u32 text_i = 0;
        u32 text_s = dirtyText.length();

        /* does not allow regular spaces, tabs or any returns at the beginning of the cleanText */
        while (text_i != text_s && IsWhitespace(dirtyText[text_i])) {
            text_i++;
        }

        /* changes tabs to spaces, but does not allow returns inside the cleanText */
        while (text_i != text_s) {
            if (dirtyText[text_i] == '\n' || dirtyText[text_i] == '\r') {
                cleanText += ' ';
            }
            elif (dirtyText[text_i] != '\t') {
                cleanText += dirtyText[text_i];
            }
            text_i++;
        }

        /* changes multiple adjacent spaces into a single space */
        string cleanSingleSpaceText = "";
        u32 cleanText_s = cleanText.length();
        text_i = 0;
        while (text_i != cleanText_s) {
            if (cleanText[text_i] == ' ') {
                cleanSingleSpaceText += cleanText[text_i];
                text_i++;
                while (text_i != text_s && cleanText[text_i] == ' ') {
                    text_i++;
                }
            }
            else {
                cleanSingleSpaceText += cleanText[text_i];
                text_i++;
            }
        }
        return cleanSingleSpaceText;
    }

    string DisplayTextInterpret (string text) {
        string displayText = "";
        u32 text_i = 0;
        u32 text_s = text.length();

        while (text_i != text_s) {
            if (text[text_i] == '\\') {
                text_i++;
                if (text_i == text_s) {
                    break;
                }   
                string specialChar = "";
                switch (text[text_i]) { /* special characters are preceded with a '\' character: '\A'  ->  '&' */
                    case 'A': specialChar = "&";       break;
                    case 'B': specialChar = "\\";      break;
                    case 'C': specialChar = "%";       break;
                    case 'D': specialChar = "$";       break;
                    case 'H': specialChar = "#";       break;
                    case 'J': specialChar = "\u2060";  break; /* word-joiner */
                    case 'M': specialChar = "@";       break;
                    case 'N': specialChar = "\u00A0";  break; /* non-breaking space */
                    case 'P': specialChar = "|";       break;
                    case 'S': specialChar = " ";       break;
                    case 'T': specialChar = "~";       break;
                    case '1': specialChar = "[";       break;
                    case '2': specialChar = "]";       break;
                    case '3': specialChar = "{";       break;
                    case '4': specialChar = "}";       break;
                    default: Error("Incorrect special character declaration."); break;
                }
                if (specialChar != "") {
                    displayText += specialChar;
                }
            }
            else {
                displayText += text[text_i];
            }
            text_i++;
        }
        /* @TODO text formatting? *text* would be cursive, _text_ would be bold */
        return displayText;
    }

    string WrapText (string unwrapped, u32 width) {
        string wrapped = "";
        u32 currentColumn_i = 0;
        string currentWord = "";
        u32 unwrapped_s = unwrapped.length();
        if (width != 0) {
            for (u32 i = 0; i < unwrapped_s; i++) {
                currentWord += unwrapped[i];
                if (unwrapped[i] == ' ' && currentWord.length() > 3) { /* if a word has more than two characters and comes across a space */
                    wrapped += currentWord;
                    currentWord = "";
                }
                if (currentWord.length() == width) { /* break it with at least 4 characters in new line */
                    if (width >= 10) {
                        wrapped += currentWord.substr(0, width - 4);
                        char lastBreakChar = wrapped.back();
                        if (lastBreakChar != ' ' && lastBreakChar != '-') { /* @TODO non-breaking space character too? */
                            wrapped += '-';
                        }
                        wrapped += '\n';
                        currentWord = currentWord.substr(width - 4, 4);
                        currentColumn_i = 4;
                    }
                    else {
                        wrapped += currentWord + '\n';
                        currentWord = "";
                        currentColumn_i = 0;
                    }
                }

                if (currentColumn_i == width) {
                    wrapped += '\n';
                    currentColumn_i = currentWord.length();
                }
                currentColumn_i++;
            }
            if (currentWord != "") {
                wrapped += currentWord;
            }
        }
        else {
            wrapped = unwrapped;
        }
        
        return wrapped;
    }

    bool IsTextVisible (string text) { /* determines whether the text has any non-whitespace character */
        u32 text_i = 0; u32 text_s = text.length();
        while (text_i != text_s && IsWhitespace(text[text_i])) {
            text_i++;
        }
        return (text_i != text_s);
    }


// // // SCAN, CHECK, SEEK FUNCTIONS // // //


    void SeekUntil (char* txt, u32& t_i, string endingChars) { /* increments the text index until it comes across one of the characters inside the endingChars argument */
        u32 chars_s = endingChars.length(); u32 i = 0;
        while (true) {
            for (i = 0; i < chars_s; i++) {
                if (txt[t_i] == endingChars[i]) { return; }
            }
            t_i++;
        }
    }

    void SeekEndOfConditional (char* txt, u32& t_i) {
        /* &...&*.......||^..   * - starts here  ,  ^ - finishes there */
        int nestingDepth_b = 0;
        while (true) {
            SeekUntil(txt, t_i, "&|");
            if (txt[t_i] == '&') { /* &...& */
                nestingDepth_b++;
                SeekEndOfStatement(txt, t_i, "&");
            }
            elif (txt[t_i] == '|' && txt[t_i + 1] == '~') { /* |~ */
                Error("A conditional doesn't have its corresponding '||' symbol.", txt, t_i);
                break;
            }
            elif (txt[t_i] == '|' && txt[t_i + 1] == '|') { /* || */
                t_i += 2;
                if (nestingDepth_b != 0) { nestingDepth_b--; }
                else { break; } /* it finds the corresponding conditional ending here */
            }
            elif (txt[t_i] == '|') { /* | */
                t_i++;
            }
        }
    }

    void SeekEndOfChoiceRange (char* txt, u32& t_i) {
        /* seeks end of 'current' choice range we are in */
        /* ...{...*...{..}..{..{}..}..{..}..}.... */
        int nestingDepth_b = 0;
        u32 pos_i = t_i;
        while (true) {
            SeekUntil(txt, t_i, "{}|");
            if (txt[t_i] == '{') {
                t_i++;
                SeekUntil(txt, t_i, "{}");

                if (txt[t_i] == '{') { /* {...{...}.... */
                    nestingDepth_b++;
                    SeekEndOfStatement(txt, t_i, "}"); /* {...{...}*... */
                }
                elif (txt[t_i] == '}') { /* {...}..... */
                    t_i++;
                    pos_i = t_i;
                }
            }
            elif (txt[t_i] == '}') { /* ....}.... */
                t_i++;
                if (nestingDepth_b != 0) { nestingDepth_b--; }
                else { break; }
            }
            elif (txt[t_i] == '|' && txt[t_i + 1] == '~') {
                t_i = pos_i;
                Error("The choice isn't in any choice range.", txt, t_i);
                break;
            }
            elif (txt[t_i] == '|') {
                t_i++;
            }
        }
        /* ...{.......{..}..{..{}..}..{..}..}*... */
    }

    void SeekEndOfStatement (char* txt, u32& t_i, string endingChar) {
        t_i++; /* ?*....?.... */
        SeekUntil(txt, t_i, endingChar);
        t_i++; /* ?.....?*... */
    }


    string ScanTextUntil (char* txt, u32& t_i, string endingChars) { /* return text until certain characters, modifies the t_i index! */
        /* ! - beginning char   ,   ? - endingChar   ,   * - caret position   ,   . - text we want to store */
        t_i++; /* !*....? */
        string storedText = "";
        u32 chars_s = endingChars.length(); u32 i = 0;
        while (true) {
            for (i = 0; i < chars_s; i++) {
                if (txt[t_i] == endingChars[i]) {
                    t_i++; /* !.....?* */
                    return RemoveWhitespace(storedText);
                }
            }
            storedText += txt[t_i];
            t_i++;
        }
    }


    /* checks for following scenario and returns true if so (can be any number of conditionals "&...&" before the "{...}" sequence): */
    /* &...&&...&&...&{...}..... */
    bool IsItConditionalChoice (char* txt, u32 t_i) {
        /* * - caret position   ,   . - some text */
        /* &...&*...&&...&..... */       /* text index is at the caret position here, it is behind a '&' symbol */
        while (IsWhitespace(txt[t_i])) {
            t_i++;
        }
        if (txt[t_i] == '&') {
            while (true) {
                SeekEndOfStatement(txt, t_i, "&");
                while (IsWhitespace(txt[t_i])) {
                    t_i++;
                }
                if (txt[t_i] != '&') {
                    break;
                }
            }
        }
        /* &...&&...&&...&*.... */
        if (txt[t_i] == '{') {
            t_i++;
            SeekUntil(txt, t_i, "{}");
            if (txt[t_i] == '}') {
                return true;
            }
        }
        return false;
    }


// // // INSTRUCTION INTERPRETERS // // //


    enum Operator { /* operator, its assigned number is the precedence */
        OP_NONE,
        OP_OR,OP_AND,
        OP_EQ,OP_NEQ,OP_GT,OP_GE,OP_LT,OP_LE, /* for condInterpreter */
        OP_ADD,OP_SUB,OP_MUL,OP_DIV,OP_MOD,OP_POW,  /* for both */
        OP_NOT,OP_LEN,OP_STR,OP_MIN,OP_MAX,OP_SUBSTR,
        OP_COMMA,OP_LP,OP_NLP,OP_RP,
        OP_IS,OP_EADD,OP_ESUB,OP_EMUL,OP_EDIV,OP_EMOD,OP_EPOW  /* for varInterpreter */
    };

    Operator StringToOperator (string opText) {
        /* for condInterpreter */
        if (opText == "OR")  { return OP_OR;  }
        if (opText == "AND") { return OP_AND; }
        if (opText == "(")   { return OP_LP;  }
        if (opText == "!(")  { return OP_NLP; }
        if (opText == ")")   { return OP_RP;  }
        if (opText == "==")  { return OP_EQ;  }
        if (opText == "!=")  { return OP_NEQ; }
        if (opText == ">")   { return OP_GT;  }
        if (opText == ">=")  { return OP_GE;  }
        if (opText == "<")   { return OP_LT;  }
        if (opText == "<=")  { return OP_LE;  }
        /* for varInterpreter */
        if (opText == "=")   { return OP_IS;   }
        if (opText == "+=")  { return OP_EADD; }
        if (opText == "-=")  { return OP_ESUB; }
        if (opText == "*=")  { return OP_EMUL; }
        if (opText == "/=")  { return OP_EDIV; }
        if (opText == "%=")  { return OP_EMOD; }
        if (opText == "^=")  { return OP_EPOW; }
        /* for both */
        if (opText == "+")   { return OP_ADD;   }
        if (opText == "-")   { return OP_SUB;   }
        if (opText == "*")   { return OP_MUL;   }
        if (opText == "/")   { return OP_DIV;   }
        if (opText == "%")   { return OP_MOD;   }
        if (opText == "^")   { return OP_POW;   }
        if (opText == ",")   { return OP_COMMA; }
        if (opText == "NOT") { return OP_NOT;   }
        if (opText == "LEN") { return OP_LEN;   }
        if (opText == "STR") { return OP_STR;   }
        if (opText == "MIN") { return OP_MIN;   }
        if (opText == "MAX") { return OP_MAX;   }
        if (opText == "SUBSTR") { return OP_SUBSTR;   }
        
        return OP_NONE;
    }
    
    int OperatorPrecedence (Operator op) {
        switch (op) {
            case OP_OR:                               return 1;
            case OP_AND:                              return 2;
            case OP_EQ:  case OP_NEQ: case OP_GT: 
            case OP_GE:  case OP_LT:  case OP_LE:     return 3;
            case OP_ADD: case OP_SUB:                 return 4;
            case OP_MUL: case OP_DIV: case OP_MOD:    return 5;
            case OP_POW:                              return 6;
            case OP_NOT: case OP_LEN: case OP_STR:
            case OP_MIN: case OP_MAX: 
            case OP_SUBSTR:                           return 7;
            default:                                  return 0;
        }
    }
    
    std::pair<int,string> Operation (State* state, std::pair<int,string> elementA, Operator op, std::pair<int,string> elementB) {
        int number = 0;
        switch (op) {
            case OP_EQ:  number = (int)(elementA.first == elementB.first); break;
            case OP_NEQ: number = (int)(elementA.first != elementB.first); break;
            case OP_AND: number = (int)(elementA.first && elementB.first); break;
            case OP_OR:  number = (int)(elementA.first || elementB.first); break;
            case OP_GT:  number = (int)(elementA.first >  elementB.first); break;
            case OP_GE:  number = (int)(elementA.first >= elementB.first); break;
            case OP_LT:  number = (int)(elementA.first <  elementB.first); break;
            case OP_LE:  number = (int)(elementA.first <= elementB.first); break;
            case OP_ADD: number = (elementA.first + elementB.first); break;
            case OP_SUB: number = (elementA.first - elementB.first); break;
            case OP_MUL: number = (elementA.first * elementB.first); break;
            case OP_DIV: if (elementB.first != 0) { number = (elementA.first / elementB.first); } else { /* @TODO error divided by zero*/ } break;
            case OP_MOD: number = (elementA.first % elementB.first); break;
            case OP_POW: number = (int)pow(elementA.first, elementB.first); break;
            case OP_MIN: number = std::min(elementA.first, elementB.first); break;
            case OP_MAX: number = std::max(elementA.first, elementB.first); break;
            default: {
                Error("Wrong operator inside the integer instruction.", state->text, state->currentPos.text_i);
                state->condElse[state->currentPos.condNestingDepth] = true;
                return std::make_pair(number, "");
                break;
            }
        }
        return std::make_pair(number, "");
    }
    
    bool IsFunctionOperator (Operator op) {
        if (op == OP_NOT || op == OP_LEN || op == OP_STR || op == OP_MIN || op == OP_MAX || op == OP_SUBSTR) { return true; }
        return false;
    }
    
    bool IsRightAssociativeOperator (Operator op) {
        if (IsFunctionOperator(op) || op == OP_POW || op == OP_AND || op == OP_OR) { return true; }
        return false;
    }
    
    bool IsSingleArgumentOperator (Operator op) {
        if (op == OP_NOT || op == OP_LEN || op == OP_STR) { return true; }
        return false;
    }
    
    bool IsTripleArgumentOperator (Operator op) {
        if (op == OP_SUBSTR) { return true; }
        return false;
    }

    vector<string> SplitInstrSegments (string instrText) { /* splits all instruction segments: "!(Var1 == Var2)"  ->  "!(" "Var1" "==" "Var2" ")" */
        vector<string> segments;
        u32 i = 0;
        u32 instrText_s = instrText.size();
        while (i + 1 < instrText_s) {
            if ((instrText[i] == '(' && instrText[i + 1] != ' ') || 
                (instrText[i] != ' ' && instrText[i] != '!' && instrText[i + 1] == '(') || 
                (i + 2 < instrText_s && instrText[i] != ' ' && instrText[i + 1] == '!' && instrText[i + 2] == '(') || 
                (instrText[i] != ' ' && instrText[i + 1] == ')') || 
                (instrText[i] == ')' && instrText[i + 1] != ' ') || 
                (instrText[i] != ' ' && instrText[i + 1] == ',') || 
                (instrText[i] == ',' && instrText[i + 1] != ' ')) {
                instrText.insert(i + 1, " ");
                instrText_s++;
            }
            i++;
        }
        if (instrText.size() != 0 && instrText.back() == ' ') {
            instrText.pop_back(); /* remove a space at the end (if there's one) */
        }
        i = 0; instrText_s = instrText.size(); 
        string segmentText_b = ""; 
        while (true) {
            while (i != instrText_s && instrText[i] != ' ' && instrText[i] != '"') { /* @TODO add the (') sign here too */
                segmentText_b += instrText[i];
                i++;
            }
            if (i != instrText_s && instrText[i] == '"') {
                segmentText_b += instrText[i];
                i++;
                while (i != instrText_s && instrText[i] != '"') { /* @TODO add the (') sign here too */
                    segmentText_b += instrText[i];
                    i++;
                }
                if (i != instrText_s) {
                    segmentText_b += instrText[i];
                    i++;
                    if (i != instrText_s && instrText[i] != ' ') {
                        continue;
                    }
                }
            }
            segments.push_back(segmentText_b);
            segmentText_b = "";
            if (i == instrText_s) { break; }
            i++;
        }
        return segments;
    }

    std::pair<int,string> GetValue (State* state, string varText) { /* checks whether the string is a variable or a plain number, and then gets the integer value of said number or the value behind the variable */
        bool isNegated = false;
		if (varText.length() != 0 && varText[0] == '!') {
			isNegated = true;
			varText = varText.substr(1, varText.length() - 1);
		}
		
		bool hasSucceeded;
        int converted = stringToInt(varText, hasSucceeded);
        if (hasSucceeded) { /* conversion success, it is a number */
			if (isNegated) {
				return std::make_pair((int)(!converted), "");				
			}
			else {
				return std::make_pair(converted, "");				
			}
        }
        else {             /* conversion fail, it is a variable */
            u32 varText_s = varText.length();
            if (varText_s >= 2 && varText[0] == '"' && varText[varText_s - 1] == '"') { /* @TODO add (') characters alongside (") too */
                string stringVal = varText.substr(1, varText_s - 2);
                return std::make_pair(0, stringVal);
            }
            return GetVar(state, varText, isNegated);
        }
    }
    
    std::pair<int,string> OperationsInterpret (State* state, vector<string> segments) {
        u32 segments_s = segments.size();
        u32 segment_i = 0;
        
        vector<string> output;
        vector<string> operators;
        while (segment_i != segments_s) {
            Operator op = StringToOperator(segments[segment_i]);
            if (op == OP_NONE) { /* is a value */
                output.push_back(segments[segment_i]);
            }
            elif (IsFunctionOperator(op)) {
                operators.push_back(segments[segment_i]);
            }
            elif (op == OP_COMMA) {
                while (!operators.empty() && StringToOperator(operators.back()) != OP_LP) {
                    output.push_back(operators.back());
                    operators.pop_back();
                }
                if (operators.empty()) {
                    Error("Lone comma outside of function parentheses.", state->text, state->currentPos.text_i);
                    return std::make_pair(0, "");
                }
            }
            elif (op == OP_NLP) {
                operators.push_back("NOT");
                operators.push_back("(");
            }
            elif (op == OP_LP) {
                operators.push_back(segments[segment_i]);
            }
            elif (op == OP_RP) {
                while (!operators.empty() && StringToOperator(operators.back()) != OP_LP) {
                    output.push_back(operators.back());
                    operators.pop_back();
                }
                if (!operators.empty()) { // @TODO ? should be without ! ??
                    operators.pop_back();
                    if (!operators.empty() && IsFunctionOperator(StringToOperator(operators.back()))) {
                        output.push_back(operators.back());
                        operators.pop_back();
                    }
                }
                else {
                    Error("Mismatched parentheses.", state->text, state->currentPos.text_i);
                    return std::make_pair(0, "");
                }
            }
            else { /* is a non-parenthesis operator */
                Operator op2 = OP_NONE;
                if (!operators.empty()) {
                    op2 = StringToOperator(operators.back());
                }
                while (!operators.empty() && op2 != OP_LP && ( OperatorPrecedence(op2) > OperatorPrecedence(op) || ( OperatorPrecedence(op2) == OperatorPrecedence(op) && !IsRightAssociativeOperator(op) ) )) {
                    output.push_back(operators.back());
                    operators.pop_back();
                    if (!operators.empty()) {
                        op2 = StringToOperator(operators.back());
                    }
                    else {
                        break;
                    }
                }
                operators.push_back(segments[segment_i]);
            }
            
            segment_i++;
        }

        while (!operators.empty()) {
            Operator op = StringToOperator(operators.back());
            if (op == OP_LP || op == OP_RP) {
                Error("Mismatched parentheses.", state->text, state->currentPos.text_i);
                return std::make_pair(0, "");
            }
            output.push_back(operators.back());
            operators.pop_back();
        }
        
        
        u32 output_i = 0;
        u32 output_s = output.size();
        vector<string>                stackSegments;
        vector<std::pair<int,string>> stackValues;
        std::pair<int,string> elementA, elementB, elementC;
        while (output_i != output_s) {
            Operator op = StringToOperator(output[output_i]);
            if (op == OP_NONE) {
                stackSegments.push_back(output[output_i]); stackValues.push_back(GetValue(state, output[output_i]));
            }
            else {
                if (IsSingleArgumentOperator(op)) {
                    if (stackValues.empty()) { goto OperationError; } elementA = stackValues.back(); stackValues.pop_back(); stackSegments.pop_back();
                    switch (op) {
                        case OP_NOT: {
                            stackSegments.push_back("0"); stackValues.push_back(std::make_pair((int)(!(elementA.first)), ""));
                            break;
                        }
                        case OP_LEN: {
                            stackSegments.push_back("0"); stackValues.push_back(std::make_pair((elementA.second).length(), ""));
                            break;
                        }
                        case OP_STR: {
                            stackSegments.push_back("0"); stackValues.push_back(std::make_pair(0, std::to_string(elementA.first)));
                            break;
                        }
                        default: break;
                    }
                }
                elif (IsTripleArgumentOperator(op)) {
                    if (stackValues.empty()) { goto OperationError; } elementA = stackValues.back(); stackValues.pop_back(); stackSegments.pop_back();
                    if (stackValues.empty()) { goto OperationError; } elementB = stackValues.back(); stackValues.pop_back(); stackSegments.pop_back();
                    if (stackValues.empty()) { goto OperationError; } elementC = stackValues.back(); stackValues.pop_back(); stackSegments.pop_back();
                    switch (op) {
                        case OP_SUBSTR: {
                            if (elementB.first >= 0 && elementB.first <= (int)(elementC.second).length()) {
                                stackSegments.push_back("0"); stackValues.push_back(std::make_pair(0, (elementC.second).substr(elementB.first, elementA.first)));
                            }
                            else {
                                Error("Second argument in a substring operation is invalid.", state->text, state->currentPos.text_i);
                                state->condElse[state->currentPos.condNestingDepth] = true;
                                return std::make_pair(0, "");
                            }
                            break;
                        }
                        default: break;
                    }
                }
                else {
                    bool isString = false;
                    if (stackValues.empty()) { goto OperationError; } elementA = stackValues.back(); stackValues.pop_back();
                    if (stackValues.empty()) { goto OperationError; } elementB = stackValues.back(); stackValues.pop_back();
                    if (elementA.second != "" || elementB.second != "") {
                        isString = true;
                    }
                    if (stackSegments.back() == "\"\"") { isString = true; } stackSegments.pop_back();
                    if (stackSegments.back() == "\"\"") { isString = true; } stackSegments.pop_back();
                    
                    if (isString) { /* @TODO include (') characters for quotation */
                        switch (op) {
                            case OP_ADD: stackSegments.push_back("\"" + elementB.second + elementA.second + "\""); stackValues.push_back(std::make_pair(0, elementB.second + elementA.second)); break;
                            case OP_EQ:  stackSegments.push_back("0"); stackValues.push_back(std::make_pair((int)(elementB.second == elementA.second), "")); break;
                            case OP_NEQ: stackSegments.push_back("0"); stackValues.push_back(std::make_pair((int)(elementB.second != elementA.second), "")); break;
                            default: {
                                Error("Wrong operator inside the string instruction.", state->text, state->currentPos.text_i);
                                state->condElse[state->currentPos.condNestingDepth] = true;
                                return std::make_pair(0, "");
                            }
                        }
                    }
					else {
						// @TODO check for errors
						if (IsRightAssociativeOperator(op)) {
							stackSegments.push_back("0"); stackValues.push_back(Operation(state, elementA, op, elementB));
						}
						else {
							stackSegments.push_back("0"); stackValues.push_back(Operation(state, elementB, op, elementA));
						}
					}
                }
            }    
            output_i++;
        }
        
        if (stackValues.empty()) {
            return std::make_pair(0, "");
        }
        else {
            return stackValues.back();            
        }
        
        OperationError:
        
        Error("Invalid operation.", state->text, state->currentPos.text_i);
        state->condElse[state->currentPos.condNestingDepth] = true;
        return std::make_pair(0, "");
    }


    void VarInstrInterpret (State* state, string instrText) { /* variable instructions interpreter */
        vector<string> segments = SplitInstrSegments(instrText);
        if (segments.size() == 1) { /* shortcuts used for setting variable to true/false or incrementing/decrementing */
            u32 segment_s = segments[0].length();
            if (segment_s >= 2 && segments[0][segment_s - 1] == '+' && segments[0][segment_s - 2] == '+') {
                GetVar(state, segments[0].substr(0, segment_s - 2), false).first++; /* increment if ends with the '++' characters */
            }
            elif (segment_s >= 2 && segments[0][segment_s - 1] == '-' && segments[0][segment_s - 2] == '-') {
                GetVar(state, segments[0].substr(0, segment_s - 2), false).first--; /* decrement if ends with the '--' characters */
            }
            elif (segment_s >= 1 && segments[0][0] == '!') {
                GetVar(state, segments[0].erase(0, 1), false).first = 0;            /* '!' at beginning sets to false; erase() gets rid of '!' character */
            }
            else {
                GetVar(state, segments[0], false).first = 1;                        /* otherwise sets to true */
            }
        }
        elif (segments.size() >= 3) {
            string lVal = segments[0];
            Operator op = StringToOperator(segments[1]);
            if (op == OP_NONE) {
                Error("No left-hand side value in a variable instruction.", state->text, state->currentPos.text_i);
                return;
            }
            segments.erase(segments.begin()); segments.erase(segments.begin());
            std::pair<int,string> rVal = OperationsInterpret(state, segments);
            if (rVal.second != "") { /* @TODO include (') also alongside (")  */
                switch (op) {
                    case OP_IS:   GetVar(state, lVal, false).second =  rVal.second; break;
                    case OP_EADD: GetVar(state, lVal, false).second += rVal.second; break;
                    default: Error("Wrong operator inside the string variable instruction.", state->text, state->currentPos.text_i); break;
                }
            }   
            else {
                GetVar(state, lVal, false).second = "";
                switch (op) {
                    case OP_IS:   GetVar(state, lVal, false).first =  rVal.first; break;
                    case OP_EADD: GetVar(state, lVal, false).first += rVal.first; break;
                    case OP_ESUB: GetVar(state, lVal, false).first -= rVal.first; break;
                    case OP_EMUL: GetVar(state, lVal, false).first *= rVal.first; break;
                    case OP_EDIV: GetVar(state, lVal, false).first /= rVal.first; break;
                    case OP_EMOD: GetVar(state, lVal, false).first %= rVal.first; break;
                    default: Error("Wrong operator inside the integer variable instruction.", state->text, state->currentPos.text_i); break;
                }
            }
        }
    }

    void SpecInstrInterpret (State* state, string instrText) { /* special instructions interpreter */
        vector<string> segments = SplitInstrSegments(instrText);
        u32 segments_s = segments.size();

        bool wasCommandFound = false;
        bool hasEnoughArguments = true;
        u32 textLength = segments[0].size();
        while (textLength != 0 && !wasCommandFound) {
            string textSegment = segments[0].substr(0, textLength);


            /* displays number in the text itself: "#Money = 50#I have @DISPLAY Money@ dollars."  ->  "I have 50 dollars." */
            if (textSegment == string("DISPLAY").substr(0, textLength)) {
                if (segments_s < 2) { hasEnoughArguments = false; break; }
                segments.erase(segments.begin());
                std::pair<int,string> varText = OperationsInterpret(state, segments);
                if (varText.second != "") {
                    state->displayText += varText.second;
                }
                else {
                    state->displayText += std::to_string(varText.first);
                }
                wasCommandFound = true;
            }
            /* saves the game to a file */
            elif (textSegment == string("SAVE").substr(0, textLength)) {
                // StateSave(state); /* @TODO */
                wasCommandFound = true;
            }
            /* resets the values of 'temporary' variables (those starting with lowercase) */
            elif (textSegment == string("RESET").substr(0, textLength)) {
                /*ResetVars();*/
                wasCommandFound = true;
            }
            elif (textSegment == string("WAIT").substr(0, textLength)) {
                if (segments_s < 2) { hasEnoughArguments = false; break; }
                string waitNumberText = segments[1];
                bool hasSucceeded; u32 waitNumber = (u32)stringToInt(waitNumberText, hasSucceeded);
                if (hasSucceeded) {
                    waitNumber = waitNumber; /* @TODO remove later */
                }
                else {
                    Error("Following number could not be interpreted inside the special instruction: " + waitNumberText, state->text, state->currentPos.text_i);
                }
                wasCommandFound = true;
            }
            textLength--;
        }
        if (!wasCommandFound)    { Error("Unspecified command inside the special instruction.", state->text, state->currentPos.text_i); }
        if (!hasEnoughArguments) { Error("Not enough arguments inside the special instruction.", state->text, state->currentPos.text_i); }
    }

    void PersCondInstrInterpret (State* state, string instrText) { /* $[5] Count < 5$ */
        bool shouldDeactivate = false;
        if (instrText.length() != 0 && instrText[0] == '~') {
            instrText.erase(0, 1); /* removes the '~' character used for the skip */
            shouldDeactivate = true;
        }
        u32 instrText_s = instrText.length();
        u32 instrText_i = 0;

        string jumpPointInstrText = "";

        while (instrText_i != instrText_s && instrText[instrText_i] != '[') { instrText_i++; }
        if (instrText_i == instrText_s) { return; }
        instrText_i++;
        while (instrText_i != instrText_s && instrText[instrText_i] != ']') { jumpPointInstrText += instrText[instrText_i]; instrText_i++; }
        if (instrText_i == instrText_s) { return; }
        instrText_i++;

        while (instrText_i != instrText_s && instrText[instrText_i] == ' ') { instrText_i++; }
        if (instrText_s == instrText_i) { return; }
        
        string condInstrText = instrText.substr(instrText_i, instrText.length());
        if (shouldDeactivate) { // removes/deactivates the persistent conditional
            u32 persCond_s = state->persCond.size();
            for (u32 i = 0; i < persCond_s; i++) {
                if (state->persCond[i].first == jumpPointInstrText && state->persCond[i].second == condInstrText) {
                    state->persCond.erase(state->persCond.begin() + i);
                    break;
                }
            }
        }
        else { // activates the persistent conditional
            state->persCond.push_back(std::pair<string,string>(jumpPointInstrText, condInstrText));
        }
    }
    
    void JumpPointInstrInterpret (State* state, string instrText) {
        u32 instrText_s = instrText.length();
        if (instrText_s != 0 && instrText[0] == '~') {
            u32 jumpHistory_s = state->jumpHistory.size();
            
            if (instrText_s == 1) {
                if (jumpHistory_s != 0) {
                    state->currentPos = (state->jumpHistory[jumpHistory_s - 1]).second;
                    state->condElse.clear(); /* resets the ELSE statement on all depths */
                }
            }
            else {
                instrText = instrText.substr(1, instrText_s - 1);
            
                bool hasSucceeded; int jumpPointNumber_i = stringToInt(instrText, hasSucceeded);
                if (hasSucceeded) {
                    if (state->jumpBasePos.count(jumpPointNumber_i) != 0) { /* checks if there's a jump base with such number */
                        for (u32 i = jumpHistory_s - 1; i >= 0; i--) {
                            if ((state->jumpHistory[i]).first == jumpPointNumber_i) {
                                state->currentPos = (state->jumpHistory[i]).second;
                                state->condElse.clear(); /* resets the ELSE statement on all depths */
                                break;
                            }
                        }
                    }
                    else {
                        Error("Couldn't perform the jump, there's no jump base with the following number: " + instrText, state->text, state->currentPos.text_i);
                    }
                }
                else {
                    Error("Following jump point number could not be interpreted: " + instrText, state->text, state->currentPos.text_i);
                }
            }
        }
        else {
            bool hasSucceeded; int jumpPointNumber_i = stringToInt(instrText, hasSucceeded);
            if (hasSucceeded) {
                if (state->jumpBasePos.count(jumpPointNumber_i) != 0) { /* checks if there's a jump base with such number */
                    state->jumpHistory.push_back(std::pair<int,Pos>(jumpPointNumber_i, state->currentPos));
                    state->currentPos = state->jumpBasePos[jumpPointNumber_i];
                    state->condElse.clear(); /* resets the ELSE statement on all depths */
                }
                else {
                    Error("Couldn't perform the jump, there's no jump base with the following number: " + instrText, state->text, state->currentPos.text_i);
                }
            }
            else {
                Error("Following jump point number could not be interpreted: " + instrText, state->text, state->currentPos.text_i);
            }
        }
    }

    bool CondInstrInterpret (State* state, string instrText) {  /* conditional instructions interpreter, returns if the condition is true or false */
        if (instrText.length() != 0 && instrText[0] == '~') {
            instrText.erase(0, 1); /* removes the '~' character used for the skip */
        }

        vector<string> segments = SplitInstrSegments(instrText);

        while (state->currentPos.condNestingDepth >= (int)state->condElse.size()) {
            state->condElse.push_back(false);
        }
        if (segments[0] == "ELSE") {
            if (state->condElse[state->currentPos.condNestingDepth] == false) {
                return false;
            }
            else {
                segments.erase(segments.begin()); /* removes the 'ELSE' segment on the beginning */
            }
        }
        if (segments.size() == 0) {
            state->condElse[state->currentPos.condNestingDepth] = false;
            return true;
        }

        bool result = (bool)OperationsInterpret(state, segments).first;
        state->condElse[state->currentPos.condNestingDepth] = !result; /* "ELSE" is always an opposite of the result */
        return result;
    }
    
    void AddTextObject (State* state, string text, TextType type) {
        if (state == nullptr) { return; }
        TextObject textObj;
        textObj.actor_n = state->actor_n;
        textObj.text    = text;
        textObj.type    = type;
        
        state->textObjs.push_back(textObj);
    }
    
    void ShowRefreshedText (State* state) {
        if (state == nullptr) { return; }
        system("cls");
        for (u32 i = 0; i < state->textObjs.size(); i++) {
            std::cout<<state->textObjs[i].text<<'\n';
        }
    }

    string ChoiceNumberPrefix (u32 index) {
        return "{" + std::to_string(index + 1) + "} ";
    }

    void RefreshAccentedChoices (State* state) {
        if (state == nullptr) { return; }
        u32 textObjs_s = state->textObjs.size();
        u32 choices_s = state->choices.size();
        for (u32 i = 0; i < choices_s; i++) {
            if (state->choices[i].type == TextType::CHOICE_ACCENTED) {
                if (state->choices[i].accentedOptions.count(*(state->currentAccent)) != 0) { /* does the choice has current accent */
                    state->choices[i].displayText = state->choices[i].accentedOptions[*(state->currentAccent)];
                    string choiceNumberedText = ChoiceNumberPrefix(i) + state->choices[i].displayText;
                    state->textObjs[textObjs_s - choices_s + i].text = choiceNumberedText;
                }
                else { /* the accented option is unavailable for current accent */
                    string choiceNumberedText = ChoiceNumberPrefix(i) + "<Unavailable.>";
                    state->textObjs[textObjs_s - choices_s + i].text = choiceNumberedText;
                }
            }
        }
        ShowRefreshedText(state);
    }

    void ShowText (State* state, string text) {
        if (state == nullptr) { return; }
        text = RemoveWhitespace(text);
        text = DisplayTextInterpret(text);
        text = WrapText(text, state->textWidth);
        
        AddTextObject(state, text, TextType::NORMAL);
        std::cout<<text<<std::endl;

        #ifdef DIAL_DEBUG
        isBacktrackLocked = false;
        #endif
    }

    void ShowChoices (State* state) {
        if (state == nullptr) { return; }
        string choiceText = "";
        u32 choices_s = state->choices.size();
        for (u32 i = 0; i < choices_s; i++) {
            choiceText = state->choices[i].displayText;
            choiceText = DisplayTextInterpret(choiceText);
            choiceText = WrapText(choiceText, state->textWidth);

            string numberedChoiceText = ChoiceNumberPrefix(i) + choiceText;

            AddTextObject(state, numberedChoiceText, state->choices[i].type);
            std::cout<<numberedChoiceText<<'\n';
        }
        RefreshAccentedChoices(state);
    }
    
    /* interprets the instrText and assigns values to the displayText, type and accentedOptions of state's choices */
    void ChoicesInterpret (State* state) {
        u32 choices_s = state->choices.size();
        for (u32 i = 0; i < choices_s; i++) {
            string choiceText = state->choices[i].instrText;
            u32 choiceText_s = choiceText.length();
            if (choiceText_s != 0 && choiceText[0] == '~') {
                choiceText.erase(0, 1);
            }
            
            string analysedChoiceText = "";
            /* scan for conditionals inside the choice */
            if (choiceText_s != 0) {
                State* state_b = new State();
                state_b->text = new char[choiceText_s + 3];
                std::strcpy(state_b->text, choiceText.c_str());
                state_b->text[choiceText_s] = '|';
                state_b->text[choiceText_s + 1] = '~';
                state_b->text[choiceText_s + 2] = 0;
                
                state_b->currentPos.text_i = 0;
                state_b->currentPos.condNestingDepth = 0;
                char* txt = state_b->text;
                u32& t_i = state_b->currentPos.text_i;
                while (t_i < choiceText_s) {
                    switch (txt[t_i]) {
                        case '&': { 
                            /* @TODO make REPEAT variable available, hard to make it work though without making the code dirty */
                            string instrText = ScanTextUntil(txt, t_i, "&");
                            if (IsItConditionalChoice(txt, t_i)) {
                                SeekEndOfChoiceRange(txt, t_i);
                            }
                            else {
                                bool isConditionTrue = CondInstrInterpret(state_b, instrText);
                                if (isConditionTrue) {
                                    state_b->currentPos.condNestingDepth++;
                                }
                                else {
                                    SeekEndOfConditional(txt, t_i);
                                }
                            }
                            break;
                        }
                        case '|': { 
                            if (txt[t_i + 1] == '|') { /* || */
                                t_i += 2;
                                state_b->currentPos.condNestingDepth--;
                            }
                            else { /* | */
                                analysedChoiceText += txt[t_i];
                                t_i++;
                            }
                            break;
                        }
                        default: { 
                            analysedChoiceText += txt[t_i];
                            t_i++;
                            break;
                        }
                    }
                }
                delete[] state_b->text;
                delete state_b;
            }
            
            choiceText = RemoveWhitespace(analysedChoiceText);
            TextType choiceType = TextType::CHOICE_NORMAL;
            
            if (choiceText[0] == '|') { /* accented choice */
                choiceType = TextType::CHOICE_ACCENTED;
                choiceText_s = choiceText.length();
                u32 t_i = 1;
                string accentName = "";
                string accentedText = "";
                while (t_i != choiceText_s) {
                    while (t_i != choiceText_s && choiceText[t_i] != ' ' && choiceText[t_i] != ':') {
                        accentName += choiceText[t_i];
                        t_i++;
                    }
                    if (choiceText[t_i] == ' ' || choiceText[t_i] == ':') {
                        while (t_i != choiceText_s && (choiceText[t_i] == ' ' || choiceText[t_i] == ':')) {
                            t_i++;
                        }
                        while (t_i != choiceText_s && choiceText[t_i] != '|') {
                            accentedText += choiceText[t_i];
                            t_i++;
                        }
                        if (t_i != choiceText_s && choiceText[t_i] == '|') {
                            t_i++;
                        }
                        
                        if (std::isupper(accentName[0])) { 
                            Error("Accented choice option's first character must be lowercase as it has to be a local variable.", state->text, state->currentPos.text_i);
                            break;
                        }
                        else {
                            state->localVars[accentName] = std::make_pair(0, "");
                        }
                        state->choices[i].accentedOptions[accentName] = accentedText;
                        accentName = "";
                        accentedText = "";
                    }
                    else {
                        Error("The accented choice option doesn't have its corresponding text; it might be missing a space after the accent's name.", state->text, state->currentPos.text_i);
                        break;
                    }
                }
                choiceText = "";
            }
            /* @TODO else if '%' for chance? */

            state->choices[i].displayText = choiceText;
            state->choices[i].type = choiceType;
        }
        state->possibleAccents.clear();
        bool hasAccentedChoice = false;
        for (u32 i = 0; i < choices_s; i++) {
            if (state->choices[i].type == TextType::CHOICE_ACCENTED) {
                hasAccentedChoice = true;
                for (map<string, string>::iterator it = state->choices[i].accentedOptions.begin(); it != state->choices[i].accentedOptions.end(); it++) {
                    state->possibleAccents.insert(it->first);
                }
            }
        }
        if (hasAccentedChoice) {
            state->currentAccent = state->possibleAccents.begin();
        }
    }
    
// // // EXTERNAL FUNCTIONS // // //
    bool IsCurrentStatus (State* state, dial::Status status) {
        if (state == nullptr) { return false; }
        return (state->status == status);
    }
    
    bool IsChoiceValid (State* state, int choice_i) {
        if (state == nullptr) { return false; }
        
        if (state->choices[choice_i].type == dial::TextType::CHOICE_ACCENTED) {
            std::string currentAccentName = *(state->currentAccent);
            if (state->choices[choice_i].accentedOptions.count(currentAccentName) == 0) {
                return false;
            }
        }
        return true;

    }
    void Choice (State* state, int choice_i) {
        if (state == nullptr) { return; }
        
        if (state->choices[choice_i].type == dial::TextType::CHOICE_ACCENTED) {
            std::string currentAccentName = *(state->currentAccent);
            if (state->choices[choice_i].accentedOptions.count(currentAccentName) != 0) {
                state->localVars[currentAccentName] = std::make_pair(1, "");
                state->saveData.push_back("a:" + currentAccentName);
            }
            else { /* ignores the user's choice as this accented choice is unavailable */
                return;
            }
        }
        
        int choicePos_i = state->choices[choice_i].jumpPos.text_i;
        //if (state->choices[i].instrText.length() != 0 && state->choices[i].instrText[0] == '~') {
        state->hasOneUseChoiceRecurred[choicePos_i] = true; /* @TODO change the naming here, also delete this if statement */
        //}
         
        /* deletes from textObjs until it comes across a NORMAL or CHOICE_SELECTED type */
        while (state->textObjs.size() != 0 && state->textObjs.back().type != dial::TextType::NORMAL && state->textObjs.back().type != dial::TextType::CHOICE_SELECTED) { /* deletes until texttype == normal or size is 0 */
            state->textObjs.pop_back();
        }
        /* then adds our selected choice to the textObjs pool */
        dial::AddTextObject(state, state->choices[choice_i].displayText, dial::TextType::CHOICE_SELECTED);
        
        dial::ShowRefreshedText(state);

        state->currentPos = state->choices[choice_i].jumpPos;
        state->saveData.push_back(std::to_string(choice_i + 1));
        state->status = dial::Status::INTERPRET;
        Dialogue_T(state); /* @TODO make it more elegant? */
    }
    
    void Continuation (State* state) {
        if (state == nullptr) { return; }
        
        state->saveData.push_back("0");
        state->status = dial::Status::INTERPRET;
    }
    
    void AccentIncrement (State* state) {
        if (state == nullptr) { return; }
        
        if (state->possibleAccents.size() != 0) {
            if (state->currentAccent == --state->possibleAccents.end()) { state->currentAccent = state->possibleAccents.begin(); }
            else { state->currentAccent++; }
            dial::RefreshAccentedChoices(state);
        }
    }
    
    void AccentDecrement (State* state) {
        if (state == nullptr) { return; }
        
        if (state->possibleAccents.size() != 0) {
            if (state->currentAccent == state->possibleAccents.begin()) { state->currentAccent = --state->possibleAccents.end(); }
            else { state->currentAccent--; }
            dial::RefreshAccentedChoices(state);
        }
    }
    
    bool HasOneUseChoiceRecurred (State* state, int choice_i) {
        if (state == nullptr) { return false; }
        return state->hasOneUseChoiceRecurred[state->choices[choice_i].jumpPos.text_i];
    }
    
    u32 GetChoicesSize (State* state) {
        if (state == nullptr) { return 0; }
        return state->choices.size();
    }
    
    State* State_I (string file_n) {
        State* state = new State();
        
        string fullFile_n = file_n + ".dial";

        FILE* textFile = fopen(fullFile_n.c_str(), "rb");
        if (textFile != nullptr) {
            fseek(textFile, 0, SEEK_END);
            state->text_s = ftell(textFile);
            fseek(textFile, 0, SEEK_SET);
            
            if (state->text_s < 2) {
                Error("File with the following name doesn't have at least 2 characters: " + fullFile_n);
                fclose(textFile);
                delete state;
                state = nullptr;
                return state;
            }
            
            state->saveData.push_back("f:" + file_n);

            state->text = new char[state->text_s + 1];
            fread(state->text, sizeof(char), state->text_s, textFile);
            fclose(textFile);
            state->text[state->text_s] = 0;

            /* "removing" comments */
            bool isComment = false;
            bool isInsideDoubleSlashComment = false;
            u32 commentNestingDepth = 0;
            char replaceSign = '\t';
            for (u32 i = 0; i < state->text_s; i++) { /* @TODO check the logic here thoroughly */
                if (state->text[i] == '/' && state->text[i + 1] == '*') {
                    isComment = true;
                    commentNestingDepth++;
                    state->text[i] = replaceSign;
                    state->text[i + 1] = replaceSign;
                }
                if (state->text[i] == '*' && state->text[i + 1] == '/') {
                    commentNestingDepth--;
                    if (commentNestingDepth == 0 && !isInsideDoubleSlashComment) {
                        isComment = false;
                    }
                    state->text[i] = replaceSign;
                    state->text[i + 1] = replaceSign;
                }
                if (state->text[i] == '/' && state->text[i + 1] == '/') {
                    isInsideDoubleSlashComment = !isInsideDoubleSlashComment; 
                    if (commentNestingDepth == 0 && !isInsideDoubleSlashComment) {
                        isComment = false;
                    }
                    state->text[i] = replaceSign;
                    state->text[i + 1] = replaceSign;
                    i++;
                }

                if (isComment) {
                    state->text[i] = replaceSign;
                }
            }
            
            state->textWidth = DIAL_DEFAULT_TEXT_WIDTH;
            state->displayText = "";
            state->status = Status::INTERPRET;

            state->currentPos.text_i = 0;
            state->currentPos.condNestingDepth = 0;
            
            state->seedRandom = time(0);
            srand(state->seedRandom);
            state->saveData.push_back("s:" + std::to_string(state->seedRandom));

            #ifdef DIAL_DEBUG
            if (HasDetectedCriticalErrors(state)) {
                State_D(state);  /* destroys and nulls the state as the text shouldn't be executed */
                return state;
            }
            #endif

            LoadJumpBases(state);
        }
        else {
            Error("Could not open a file with the following name: " + fullFile_n);
            delete state;
            state = nullptr;
        }
        return state;
    };

    void State_D (State*& state) { /* the & is here because the pointer is passed by reference, and thus can be nullptr'd */
        if (state == nullptr) { return; }

        delete[] state->text;
        delete state;
        state = nullptr;
    }

    void StateSave (State* state, int save_i) {
        if (state == nullptr) { return; }
        
        string file_n = "save_" + state->saveData[0].substr(2, state->saveData[1].length() - 8) + "_" + std::to_string(save_i) + ".txt"; /* @TODO .dial or .txt extension thing, change the magic numbers */
        
        FILE* textFile = fopen(file_n.c_str(), "w");
        if (textFile != nullptr) {
            for (auto& data : state->saveData) {
                fprintf(textFile, (data + ",").c_str());
            }
            fclose(textFile);
        }
        else {
            Error("Could not create a file with the following name: " + file_n);
        }
    }

    State* StateLoad (string file_n, int save_i) { /* @TODO */
        State* state = nullptr;
        
        FILE* textFile = fopen(("save_" + file_n + "_" + std::to_string(save_i) + ".txt").c_str(), "rb");
        if (textFile != nullptr) {
            fseek(textFile, 0, SEEK_END);
            u32 text_s = ftell(textFile);
            fseek(textFile, 0, SEEK_SET);
            
            char* text = new char[text_s + 1];
            fread(text, sizeof(char), text_s, textFile);
            text[text_s] = 0;
            
            u32 text_i = 0;
            
            vector<string> saveData;
            while (true) {
                string saveDataText = "";
                while (text_i != text_s) {
                    if (text[text_i] == ',') {
                        break;
                    }
                    saveDataText += text[text_i];
                    text_i++;
                }
                if (text_i == text_s) {
                    break;
                }
                if (text[text_i] == ',') {
                    saveData.push_back(saveDataText);
                    text_i++;
                }
            }
            
            state = State_I(saveData[0].substr(2, -1));
            
            vector<string> vars;
            for (auto& data : saveData) {
                Dialogue_T(state);
                if (data.length() == 0) { 
                    Error("Invalid data while loading a following file: " + string("save_") + file_n + "_" + std::to_string(save_i) + ".txt");
                    delete state;
                    state = nullptr;
                    return state;
                }
                switch (data[0]) {
                    case 'f': {
                        break;
                    }
                    case 's': {
                        state->seedRandom = std::stoul(data.substr(2, -1));
                        srand(state->seedRandom);
                        break;
                    }
                    case 'a': {
                        string accentText = data.substr(2, -1);
                        auto accent = state->possibleAccents.find(accentText);
                        if (accent != state->possibleAccents.end()) {
                            state->currentAccent = state->possibleAccents.find(accentText);
                        }
                        else {
                            Error("Invalid name of an accent while loading a following file: " + string("save_") + file_n + "_" + std::to_string(save_i) + ".txt");
                            delete state;
                            state = nullptr;
                            return state;
                        }
                        dial::RefreshAccentedChoices(state);
                        break;
                    }
                    case 'v': {
                        vars.push_back(data.substr(2, -1));
                        break;
                    }
                    case '0': {
                        for (auto& var : vars) {
                            VarInstrInterpret(state, var);
                        }
                        vars.clear();
                        Continuation(state);
                        break;
                    }
                    default: {
                        bool hasSucceeded; int choiceNumber = stringToInt(data, hasSucceeded);
                        if (hasSucceeded) {
                            if (choiceNumber > 0) {
                                bool isValid = IsChoiceValid(state, choiceNumber - 1);
                                if (!isValid) { continue; }
                                Choice(state, choiceNumber - 1);
                            }
                            else {
                                Error("Negative choice number while loading a following file: " + string("save_") + file_n + "_" + std::to_string(save_i) + ".txt");
                                delete state;
                                state = nullptr;
                                return state;
                            }
                        }
                        else {
                            Error("Couldn't recognize a symbol while loading a following file: " + string("save_") + file_n + "_" + std::to_string(save_i) + ".txt");
                            delete state;
                            state = nullptr;
                            return state;
                        }
                        break;
                    }
                }
            }
            for (auto& var : vars) {
                VarInstrInterpret(state, var);
            }
            vars.clear();
            Dialogue_T(state);
        }
        else {
            Error("Could not open a file with the following name: " + string("save_") + file_n + "_" + std::to_string(save_i) + ".txt");
        }
        return state;
    }
    
    
    



    void Dialogue_T (State* state) {
        if (state == nullptr) { return; }
        char* txt = state->text;
        u32& t_i = state->currentPos.text_i;
        int skipCond_c = 0;
        u32 jumpLoop_c = 0;
        bool hasActorNameOccured = false;

        Beginning:
        
        SaveVarDiff(state);
        switch (state->status) {
            case Status::NONE: { break; }
            case Status::WAIT_FOR_CONTINUATION: { break; }
            case Status::WAIT_FOR_CHOICE: { break; }
            case Status::INTERPRET: {
            #ifdef DIAL_DEBUG
                if (!isBacktrackLocked) {
                    isBacktrackLocked = true;
                    SaveBacktrackState(state);
                }
            #endif
                u32 persCond_s = state->persCond.size();
                for (u32 i = 0; i < persCond_s; i++) {
                    bool isConditionTrue = CondInstrInterpret(state, state->persCond[i].second);
                    if (isConditionTrue) {
                        JumpPointInstrInterpret(state, state->persCond[i].first);
                        skipCond_c = 0;
                        state->persCond.erase(state->persCond.begin() + i);
                        break;
                    }
                }

                while (true) {
                    switch (txt[t_i]) {
                        case '#': {
                            string instrText = ScanTextUntil(txt, t_i, "#");
                            VarInstrInterpret(state, instrText);
                            break;
                        }
                        case '@': {
                            string instrText = ScanTextUntil(txt, t_i, "@");
                            SpecInstrInterpret(state, instrText);
                            break;
                        }
                        case '$': {
                            string instrText = ScanTextUntil(txt, t_i, "$");
                            PersCondInstrInterpret(state, instrText);
                            break;
                        }
                        case '[': {
                            if (txt[t_i + 1] == '[') { /* [[...]], go past it */
                                SeekEndOfStatement(txt, t_i, "]");
                            }
                            else { /* [...] */
                                string instrText = ScanTextUntil(txt, t_i, "]");
                                JumpPointInstrInterpret(state, instrText);
                                skipCond_c = 0;
                                jumpLoop_c++;
                                if (jumpLoop_c >= DIAL_JUMP_LOOP_LIMIT) {
                                    Error("Infinite jump loop detected at jump point: " + instrText, txt, t_i);
                                    state->status = Status::FATAL_ERROR;
                                    goto Beginning;
                                }
                            }
                            break;
                        }
                        case ']': {
                            t_i++;
                            break;
                        }
                        case '{': {
                            t_i++;
                            SeekUntil(txt, t_i, "{}&");

                            if (txt[t_i] == '}') { /* {...} */
                                t_i++;
                                SeekEndOfChoiceRange(txt, t_i);
                            }
                            else { /* start of choice range */
                                state->choices.clear();
                                while (true) {
                                    SeekUntil(txt, t_i, "{}&|");

                                    if (txt[t_i] == '{') { /* {... */
                                        u32 possibleBeginningOfChoice_i = t_i;
                                        t_i++;
                                        SeekUntil(txt, t_i, "{}");

                                        if (txt[t_i] == '{') { /* {...{ */
                                            SeekEndOfChoiceRange(txt, t_i); /* seek the end of this new nested choice range, so that we return to our original choice range */
                                        }
                                        elif (txt[t_i] == '}') { /* {...} */
                                            t_i = possibleBeginningOfChoice_i;
                                            string choiceInstrText = ScanTextUntil(txt, t_i, "}");

                                            if (choiceInstrText.length() != 0 && choiceInstrText[0] == '~' && state->hasOneUseChoiceRecurred[t_i]) { /* if a one-use choice has already been chosen, then it is hidden */
                                                continue;
                                            }

                                            ChoiceObject choice_b;
                                            choice_b.instrText = choiceInstrText;
                                            choice_b.jumpPos.text_i = t_i;
                                            choice_b.jumpPos.condNestingDepth = state->currentPos.condNestingDepth;
                                            state->choices.push_back(choice_b);
                                        }
                                    }
                                    elif (txt[t_i] == '}') { /* ...} */
                                        if (state->choices.size() == 0) {
                                            t_i++;
                                            break;
                                        }

                                        ChoicesInterpret(state);
                                        ShowChoices(state);

                                        state->status = Status::WAIT_FOR_CHOICE;
                                        goto Beginning;
                                    }
                                    elif (txt[t_i] == '&') { /* &...& */
                                        if (IsItConditionalChoice(txt, t_i)) {
                                            while (txt[t_i] != '{') { /* loops the conditionals */
                                                string instrText = ScanTextUntil(txt, t_i, "&");
                                                bool isConditionTrue = CondInstrInterpret(state, instrText);
                                                state->condRepeat_c[t_i]++;
                                                if (isConditionTrue) {
                                                    state->currentPos.condNestingDepth++;
                                                }
                                                else {
                                                    SeekEndOfConditional(txt, t_i);
                                                    break;
                                                }
                                                while (IsWhitespace(txt[t_i])) {
                                                    t_i++;
                                                }
                                            }
                                        }
                                        else {
                                            SeekEndOfStatement(txt, t_i, "&");
                                            SeekEndOfConditional(txt, t_i);
                                        }
                                    }
                                    elif (txt[t_i] == '|' && txt[t_i + 1] == '~') { /* |~ */
                                        Error("The conditional's corresponding '||' symbol is outside the choice range it's in or the choice range is missing the ending '}' symbol.");
                                        break;
                                    }
                                    elif (txt[t_i] == '|' && txt[t_i + 1] == '|') { /* || */
                                        t_i += 2;
                                        state->currentPos.condNestingDepth--;

                                        if (state->currentPos.condNestingDepth < 0) {
                                            Error("Conditional nesting depth is below zero. There are stray '||' symbols or a corresponding conditional was not read properly.", txt, t_i);
                                            state->currentPos.condNestingDepth = 0;
                                        }
                                    }
                                    elif (txt[t_i] == '|') { /* | */
                                        t_i++;
                                    }
                                }
                            }
                            break;
                        }
                        case '}': {
                            t_i++;
                            break;
                        }
                        case '&': {
                            string instrText = ScanTextUntil(txt, t_i, "&");
                            if (IsItConditionalChoice(txt, t_i)) {
                                SeekEndOfChoiceRange(txt, t_i);
                            }
                            else {
                                bool isConditionTrue = CondInstrInterpret(state, instrText);
                                state->condRepeat_c[t_i]++;
                                if (isConditionTrue) {
                                    state->currentPos.condNestingDepth++;
                                    if (instrText.length() != 0 && instrText[0] == '~') {
                                        skipCond_c++;
                                    }
                                }
                                else {
                                    SeekEndOfConditional(txt, t_i);
                                }
                            }
                            break;
                        }
                        case '|': {
                            if (txt[t_i + 1] == '~') { /* |~ */
                                if (IsTextVisible(state->displayText)) {
                                    ShowText(state, state->displayText);
                                }
								state->displayText = "";
                                state->status = Status::FINISHED;
                                goto Beginning;
                            }
                            elif (txt[t_i + 1] == '|') { /* || */
                                t_i += 2;
                                state->currentPos.condNestingDepth--;

                                if (state->currentPos.condNestingDepth < 0) {
                                    Error("Conditional nesting depth is below zero. There are stray '||' symbols or a corresponding conditional was not read properly.", txt, t_i);
                                    state->currentPos.condNestingDepth = 0;
                                }

                                if (skipCond_c > 0) {
                                    skipCond_c--;
                                }
                                elif (IsTextVisible(state->displayText)) {
                                    ShowText(state, state->displayText);
                                    state->displayText = "";
                                    state->status = Status::WAIT_FOR_CONTINUATION;
                                    goto Beginning;
                                }
                            }
                            else { /* | */
                                t_i++;
                                if (IsTextVisible(state->displayText)) {
                                    ShowText(state, state->displayText);
                                    state->displayText = "";
                                    state->status = Status::WAIT_FOR_CONTINUATION;
                                    goto Beginning;
                                }
                            }
                            break;
                        }
                        default: {
                            if (hasActorNameOccured == false && txt[t_i] == ':') {
                                hasActorNameOccured = true;
                                state->actor_n = RemoveWhitespace(state->displayText);
                                state->displayText = "";
                                t_i++;
                                break;
                            }
                            state->displayText += txt[t_i];
                            t_i++;
                            break;
                        }
                    }
                }
                break;
            }
            case Status::FATAL_ERROR: {
                break;
            }
            case Status::FINISHED: {
                std::cout<<"\n-END OF TRANSMISSION-\n"; /* @TODO remove? */

                state->status = Status::NONE;
                break;
            }
            default: break;
        }
    }
}

#undef u32
#undef elif

#endif /* dial.hpp */

