#include <stdio.h>
#include <vector>
#include <list>
#include <stack>
#include <unordered_map>

#include "Stack.h"
#include "Function.h"

//Utility macro for getting char-delimited substrings of C strings
#define GetSubstring(term) tmp_idx = idx; for (i = 0; *tmp_idx != term && *tmp_idx != '\n' && *tmp_idx != '\r'; i++) tmp_idx++; \
                           tmp_buf = (char *) malloc(i + 1); for (size_t j = 0; j < i; j++) tmp_buf[j] = idx[j]; \
                           tmp_buf[i] = 0; idx = tmp_idx; idx++

//Utiity macro for reading from a file/stream
//I may remove this later, as it's only used in 1 place
#define ReadInput(file) char* buf = NULL; size_t n = 0; getline(&buf, &n, file); char* idx = buf

//Global boolean flags for program state management
bool ABORT = false, BYE = false, QUIT = false;

Stack *stack, *return_stack;

FILE* curr_file = NULL;

std::unordered_map<Function*, Function*> copy_map;

std::list<std::pair<std::string, Function*> > glossary;

//Oh boy, header
int cr();
int spaces();
int space();
int emit();
int add();
int sub();
int mult();
int umult();
int div();
int mod();
int modDiv();
int multDiv();
int multDivMod();
int add1();
int sub1();
int add2();
int sub2();
int abs();
int neg();
int min();
int max();
int lshift();
int rshift();
int and_();
int or_();
int swap();
int swap2();
int dup();
int dup2();
int dup_if();
int over();
int over2();
int rot();
int drop();
int drop2();
int retPush();
int retPop();
int retCopy();
int retCopy3();
int print();
int uprint();
int urjprint();
int printS();
int equals();
int lessThan();
int greaterThan();
int zeroEquals();
int zeroLessThan();
int zeroGreaterThan();
int page();
int quit();
int abort_();
int stack_q();
int cond();
int loop();
int loop_plus();
int do_();
int nop();
int leave();
void number(std::string& str);

//Finds words in the glossary
Function* find(std::string& name) {
    for(auto itr = glossary.rbegin(); itr != glossary.rend(); itr++) {
        if(itr->first == name)
            return itr->second;
    }
    return NULL;
}

//FORGET word for dictionary cleanup
char* forget(char* idx) {
    char *tmp_buf, *tmp_idx;
    size_t i;
    GetSubstring(' ');
    std::string name(tmp_buf);
    free(tmp_buf);

    auto itr = glossary.begin();
    for(; itr != glossary.end(); itr++)
        if((*itr).first == name) break;

    if(itr != glossary.end())
        glossary.erase(itr, glossary.end());

    return idx;
}

//Adds user-defined words to the glossary

//WARNING: This code is very obtuse, despite the extensive comments. I shouldn't have used so much C-style code,
// but what's done is done. I could change to using vectors, and it might make a little bit more sense, but probably not.
//Graphs are rarely clean to implement, especially due to how I built the path decider into everything. It makes for a clean
//path decider, but messy graph handling.
char* add_word(char* idx) {
    std::stack<Function*> if_stack, do_stack, begin_stack, while_stack;
    std::stack<std::vector<Function*> > leave_stack;
    char *tmp_buf;
    size_t i;
    char *tmp_idx;
    GetSubstring(' ');
    std::string name(tmp_buf), func = "";
    free(tmp_buf);
    //Declare a starting null node
    Function *head = new Function(nop), *tail = head;
    GetSubstring(' ');
    func = std::string(tmp_buf);

    while(func != ";") {
        //Comment handler
        if(func == "(") {
            GetSubstring(')');
        } else if(func == ".\"") {                       //Handles string printing
            GetSubstring('"');
            StrPrint* node = new StrPrint(tmp_buf);
            tail->next = new Function*[1];
            tail->next[0] = node;
            tail = node;
        } else if(func == "IF") {                        //Conditional handling, step 1
            Function* if_ = new Function(cond);          //Allcoate & initialize the if node
            tail->next = new Function*[1];
            tail->next[0] = if_;                         //Tail points to if
            tail->next[0]->next = new Function*[2];      //if block branching nodes declaration
            tail->next[0]->branches = 2;
            tail->next[0]->next[1] = new Function(nop);  //Dummy node for conditional branch in order to have if head node location
            if_stack.push(tail->next[0]);                //Push the node onto the conditional stack
            tail = if_stack.top()->next[1];              //Set tail node
        } else if(func == "ELSE") {
            Function* else_tail;
            else_tail = if_stack.top();                  //Grab the top of the conditional stack
            if_stack.top() = tail;                       //Move the tail
            else_tail->next[0] = new Function(nop);      //Allocate dummy node
            tail = else_tail->next[0];
            Function* then = new Function(nop);          //Another dummy node, to unite the two conditional paths
            tail->next = new Function*[1];               //Stitch the active tail into the dummy node
            tail->next[0] = then;
            if(!if_stack.top()->next)                    //Handle possible if/else
                if_stack.top()->next = new Function*[1];
            if_stack.top()->next[0] = then;              //Stitch the other tail into the dummy node
            tail = tail->next[0];
            if_stack.pop();                              //Clean up the conditional stack
        } else if(func == "THEN") {
            Function* then = new Function(nop);          //Strucutral merger node allocation
            tail->next = new Function*[1];
            tail->next[0] = then;                        //Tie the tail into the node
            if(!if_stack.top()->next)                    //Make sure the if-branch can be tied into the node
                if_stack.top()->next = new Function*[1];
            if_stack.top()->next[0] = then;              //Tie the if_stack tail into the node
            tail = tail->next[0];
            if_stack.pop();
        } else if(func == "DO") {
            Function *_do = new Function(do_);           //Allocate do function
            tail->next = new Function *[1];
            tail->next[0] = _do;                         //Set tail->next
            tail = tail->next[0];                        //Move tail
            Function *loop_head = new Function(nop);     //Allocate loop block head
            tail->next = new Function *[1];
            tail->next[0] = loop_head;                   //Move tail to loop head
            do_stack.push(loop_head);                    //Put head on do loop stack
            tail = tail->next[0];
            std::vector<Function *> temp;
            leave_stack.push(temp);                      //Set up leave stack
        } else if(func == "LEAVE") {
            Function *_leave = new Function(leave);      //Allocate leave node
            tail->next = new Function*[1];
            tail->next[0] = _leave;                      //Set tail->next
            leave_stack.top().push_back(_leave);         //Push node onto leave stack
            tail = tail->next[0];
            tail->next = new Function*[2];               //Set up leave escape path
            tail->next[0] = new Function(nop);
            tail = tail->next[0];
        } else if(func == "LOOP" || func == "+LOOP") {
            Function *loop_;
            if (func == "LOOP")
                loop_ = new Function(loop);              //Allocate loop escape checker function
            else
                loop_ = new Function(loop_plus);         //Differentiate between LOOP and +LOOP
            tail->next = new Function*[1];
            tail->next[0] = loop_;                       //Add loop escape checker to loop path
            loop_->next = new Function*[2];              //Allocate loop branching
            loop_->branches = 2;
            loop_->next[1] = do_stack.top();             //Plug loop path into loop head
            do_stack.pop();                              //Clean up do stack
            loop_->next[0] = new Function(nop);          //Set up loop escape path
            tail = loop_->next[0];                       //Move tail
            while (!leave_stack.top().empty()) {         //Take care of any leave's
                leave_stack.top().back()->next[1] = tail;
                leave_stack.top().pop_back();
            }
            leave_stack.pop();
        } else if (func == "BEGIN") {
            Function *begin = new Function(nop);         //Allocate definite loop head
            tail->next = new Function*[1];
            tail->next[0] = begin;                       //Set tail->next
            begin_stack.push(begin);                     //Push loop head onto stack
            tail = tail->next[0];                        //Move tail
        } else if (func == "UNTIL") {
            Function* until = new Function(cond);        //Allocate until conditional
            tail->next = new Function*[1];
            tail->next[0] = until;                       //Set tail->next
            tail = tail->next[0];                        //Move tail
            tail->next = new Function*[2];
            tail->next[0] = begin_stack.top();           //Point the conditional false path at loop head
            begin_stack.pop();                           //Clean up loop stack
            Function* temp = new Function(nop);          //Set up escape tail
            tail->next[1] = temp;
            tail = tail->next[1];
        } else if (func == "WHILE") {
            Function* while_ = new Function(cond);       //Allocate while conditional
            tail->next = new Function*[1];
            tail->next[0] = while_;                      //Set tail->next
            tail = tail->next[0];                        //Move tail->next
            tail->next = new Function*[2];               //Allocate branching
            Function* temp = new Function(nop);
            tail->next[1] = temp;                        //Set up true path
            temp = new Function(nop);
            tail->next[0] = temp;                        //Set up false path
            while_stack.push(temp);                      //Push false path head onto while stack
            tail = tail->next[1];                        //Move tail
        } else if (func == "REPEAT") {
            tail->next = new Function*[1];
            tail->next[0] = begin_stack.top();           //Plug current tail into loop head
            begin_stack.pop();                           //Clean up begin_stack
            tail = while_stack.top();                    //Move current tail to loop escape path
            while_stack.pop();                           //Clean up while_stack
        } else {
            copy_map.erase(copy_map.begin(), copy_map.end()); //Clean up the copy constructor map
            Function *temp;
            Function *tmp_ptr = find(func);
            temp = (Function *) (tmp_ptr ? new Function(tmp_ptr) : new Number(func));   //Number/non-Number handling
            tail->next = new Function *[1];
            tail->next[0] = temp;
            while (tail->next)                            //Integrate user-defined words properly by skipping over word graph
                tail = tail->next[0];
        }
        GetSubstring(' ');
        func = std::string(tmp_buf);
    }
    glossary.push_back(std::make_pair(name, head));
    return idx;
}

//Word-execution wrapper-executed word pointed to by func
//Written to help debugging and to avoid stack overflow resulting from excessive recursion
void run(Function* func) {
    while(func->next) {
        int idx = func->run();
        func = func->next[idx];
    }
    func->run();
}

//Carriage return
int cr() {
    printf("\n");
    return 0;
}
//Prints some number of spaces
int spaces() {
    std::string str((unsigned long)*(int*)stack->at(0), ' ');
    printf(str.c_str());
    stack->pop(1);
    return 0;
}
//Prints a space
int space() {
    printf(" ");
    return 0;
}

//Prints a character
int emit() {
    char ch = (char)*stack->at(0);
    printf("%c", ch);
    stack->pop(1);
    return 0;
}

//Polish postfix addition
int add() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) += s;
    return 0;
}
//Polish postfix subtraction
int sub() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) -= s;
    return 0;
}
//Polist postfix multiplication
int mult() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) *= s;
    return 0;
}

//Polish postfix unsigned multiplication
int umult() {
    int s = *stack->at(0);
    stack->pop(1);
    *stack->at(0) *= s;
    return 0;
}

//Polish postfix division
int div() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) /= s;
    return 0;
}
//Postfix modulo
int mod() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) %= s;
    return 0;
}
//Postfix modulo & division
int modDiv() {
    int m, s = *(int*)stack->at(0);
    stack->pop(1);
    m = *(int*)stack->at(0) % s;
    *(int*)stack->at(0) /= s;
    stack->push(m);
    return 0;
}
//Multiplies and then divides, using a long intermediate. Used for fixed-point math.
int multDiv(){
    long a = *(int*)stack->at(2), b = *(int*)stack->at(1), c = *(int*)stack->at(0);
    stack->pop(3);
    long m = a * b;
    int d = (int)(m / c);
    stack->push(d);
    return 0;
}
//Multiplies and then divides and returns the remainder, using a long intermediate. Used for fixed-point math.
int multDivMod(){
    long a = *(int*)stack->at(2), b = *(int*)stack->at(1), c = *(int*)stack->at(0);
    stack->pop(3);
    long m = a * b;
    int d = (int)(m / c), r = (int)(m % c);
    stack->push(r);
    stack->push(d);
    return 0;
}
//Increments the top of the stack
int add1() {
    (*(int*)stack->at(0))++;
    return 0;
}
//Decrements the top of the stack
int sub1() {
    (*(int*)stack->at(0))--;
    return 0;
}
//Adds 2 to the top of the stack
int add2() {
    *(int*)stack->at(0) += 2;
    return 0;
}
//Subtracts 2 from the top of the stack
int sub2() {
    *(int*)stack->at(0) -= 2;
    return 0;
}
//Leftshifts the top of the stack by 1
int lshift() {
    *(int*)stack->at(0) *= 2;
    return 0;
}
//Rightshifts the top of the stack by 1
int rshift() {
    *(int*)stack->at(0) /= 2;
    return 0;
}

//Binary and operator
int and_() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) &= s;
    return 0;
}

//Binary or operator
int or_() {
    int s = *(int*)stack->at(0);
    stack->pop(1);
    *(int*)stack->at(0) |= s;
    return 0;
}

//Computes absolute value of the top of the stack
int abs(){
    *(int*)stack->at(0) = std::abs(*(int*)stack->at(0));
    return 0;
}
//Negates the top of the stack
int neg(){
    *(int*)stack->at(0) *= -1;
    return 0;
}
//Returns minimum of 2 numbers
int min(){
    int a = *(int*)stack->at(0);
    stack->pop(1);
    int b = *(int*)stack->at(0);
    stack->pop(1);
    if(a > b)
        stack->push(b);
    else
        stack->push(a);
    return 0;
}
//Returns maximum of 2 numbers
int max(){
    int a = *(int*)stack->at(0);
    stack->pop(1);
    int b = *(int*)stack->at(0);
    stack->pop(1);
    if(a > b)
        stack->push(a);
    else
        stack->push(b);
    return 0;
}

//Swaps top two elements of the stack
int swap() {
    int t = *stack->at(0);
    stack->pop(1);
    int b = *stack->at(0);
    stack->pop(1);
    stack->push(t);
    stack->push(b);
    return 0;
}
//Swaps top two elements of the stack for the next two
int swap2() {
    long t = *(long*)stack->at(1);
    stack->pop(2);
    long b = *(long*)stack->at(1);
    stack->pop(2);
    stack->push(t);
    stack->push(b);
    return 0;
}
//Duplicates the top of the stack
int dup() {
    stack->push(*stack->at(0));
    return 0;
}
//Duplicates the top two elements of the stack
int dup2() {
    stack->push(*stack->at(1));
    stack->push(*stack->at(1));
    return 0;
}

int dup_if() {
    int q = *(int*)stack->at(0);
    if(q) stack->push(q);
    return 0;
}

//Pushes the second element of the stack onto the stack
int over() {
    int t = *stack->at(0);
    stack->pop(1);
    int d = *stack->at(0);
    stack->push(t);
    stack->push(d);
    return 0;
}
//Pushes the third and fourth elements of the stack onto the stack
int over2() {
    long t = *(long*)stack->at(1);
    stack->pop(2);
    long d = *(long*)stack->at(1);
    stack->push(t);
    stack->push(d);
    return 0;
}
//Removes the third element of the stack and pushes it onto the stack
int rot() {
    int t = *stack->at(0);
    stack->pop(1);
    int m = *stack->at(0);
    stack->pop(1);
    int b = *stack->at(0);
    stack->pop(1);
    stack->push(m);
    stack->push(t);
    stack->push(b);
    return 0;
}
//Pops the top of the stack
int drop() {
    stack->pop(1);
    return 0;
}
//Pops the top 2 elements of the stack
int drop2() {
    stack->pop(2);
    return 0;
}
//Pushes top of stack onto return stack
int retPush(){
    int a = *stack->at(0);
    stack->pop(1);
    return_stack->push(a);
    return 0;
}
//Pushes top of return stack onto stack
int retPop(){
    int a = *return_stack->at(0);
    return_stack->pop(1);
    stack->push(a);
    return 0;
}
//Copies top of return stack onto stack
int retCopy(){
    int a = *return_stack->at(0);
    stack->push(a);
    return 0;
}
//Copies 3rd value on return stack onto stack
int retCopy3(){
    int a = *return_stack->at(2);
    stack->push(a);
    return 0;
}
//Prints and then pops the top of the stack
int print() {
    printf("%d", *(int*)stack->at(0));
    stack->pop(1);
    return 0;
}

//Unsigned int print
int uprint() {
    printf("%u", *stack->at(0));
    stack->pop(1);
    return 0;
}

//Unsigned right-justified print
int urjprint() {
    uint size = *stack->at(0);
    uint data =  *stack->at(1);
    uint num_spaces = size - data/10;
    stack->pop(2);
    for(uint i = 0; i < num_spaces; i++)
        printf(" ");
    printf("%u", data);
    return 0;
}

//Prints the contents of the stack
int printS() {
    for(int a = 0; a < stack->size(); a++) {
        printf(" %d", *(int*)stack->at(a));
    }
    return 0;
}
int equals(){
    int a = *(int*)stack->at(0), b = *(int*)stack->at(1);
    stack->pop(2);
    if(a == b)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}
int lessThan(){
    int a = *(int*)stack->at(1), b = *(int*)stack->at(0);
    stack->pop(2);
    if(a < b)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}
int greaterThan(){
    int a = *(int*)stack->at(1), b = *(int*)stack->at(0);
    stack->pop(2);
    if(a > b)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}
int zeroEquals(){
    int a = *(int*)stack->at(0);
    stack->pop(1);
    if(a == 0)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}
int zeroLessThan(){
    int a = *(int*)stack->at(0);
    stack->pop(1);
    if(0 < a)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}
int zeroGreaterThan(){
    int a = *(int*)stack->at(0);
    stack->pop(1);
    if(0 > a)
        stack->push((int)0xffffffff);
    else
        stack->push(0x00000000);
    return 0;
}

//Clears the screen the same way the screen is cleared at program start
//Same logic for using system() applies.
int page() {
    system("clear");
    return 0;
}

//Aborts the program
int abort_() {
    ABORT = true;
    stack->clear();
    throw 1;
}

//Sets up the interpreter to not print ok
int quit() {
    QUIT = true;
    return 0;
}

//Returns a true flag if the stack is empty, returns false otherwise
int stack_q() {
    if(!stack->size()) stack->push((int)0xffffffff);
    else stack->push(0x00000000);
    return 0;
}

//Manages branching for if statements
//Branches to 0 if false, or to 1 if true.
int cond() {
    int a = *stack->at(0) != 0;
    stack->pop(1);
    return a;
}
//Initializes definite loops
int do_() {
    int index = *stack->at(0);
    int limit = *stack->at(1);
    stack->pop(2);
    return_stack->push(limit);
    return_stack->push(index);
    return 0;
}
//Definite loop conditional
int loop() {
    int index = *(int*)return_stack->at(0);
    int limit = *(int*)return_stack->at(1);
    index++;
    if(index != limit) {
        *(int*)return_stack->at(0) = index;
        return 1;
    } else {
        return_stack->pop(2);
        return 0;
    }
}
//Why does FORTH have to be inconsistent with its loop end condition, anyways?
int loop_plus() {
    int index = *(int*)return_stack->at(0);
    int limit = *(int*)return_stack->at(1);
    int inc = *(int*)stack->at(0);
    index += inc;
    if(inc < 0) {
        if(index >= limit) {
            *(int*)return_stack->at(0) = index;
            return 1;
        } else {
            return_stack->pop(2);
            return 0;
        }
    } else {
        if(index < limit) {
            *(int*)return_stack->at(0) = index;
            return 1;
        } else {
            return_stack->pop(2);
            return 0;
        }
    }
}

//Null operand for structural nodes
int nop() {
    return 0; //That's right; it does nothing.
}
//Nop-esque operand to handle loop break pathing
int leave() {
    return 1;
}

//Pushes a number onto the stack
void number(std::string& str) {
    if(!is_num(str)) {
        printf("%s ?\n", str.c_str());
        abort_();
        return;
    }
    int n = atoi(str.c_str());
    stack->push(n);
}

//The terminal text interpreter
void text_interpreter(char* idx) {
    while(*idx != '\0' && *idx != '\n' && *idx != '\r') {
        while (*idx == ' ') idx++;
        char *tmp_buf;
        size_t i;
        char *tmp_idx;
        GetSubstring(' ');
        std::string str(tmp_buf);
        free(tmp_buf);
        Function *func = find(str);
        if (str == "BYE")
            BYE = true;
        else if (str == ".\"") {
            GetSubstring('"');
            printf(tmp_buf);
            free(tmp_buf);
        } else if (str == ":")
            idx = add_word(idx);
        else if (str == "INCLUDE") {
            char r = 'r';
            GetSubstring(' ');
            curr_file = fopen(tmp_buf, &r);
            char *buf = NULL;
            size_t n = 0;
            while (getline(&buf, &n, curr_file) > 0) {
                char *f_idx = buf;
                text_interpreter(f_idx);
                free(buf);
                buf = NULL;
            }
            free(tmp_buf);
            fclose(curr_file);
        } else if (str == "FORGET")
            idx = forget(idx);
        else if (func)
            run(func);
        else
            number(str);
    }
}

int main() {
    system("clear"); //Yes, yes, I know, system() is evil. Then again, this isn't production code, so I don't care.
    //Initializing stack
    stack = new Stack(4096);
    return_stack = new Stack(4096);
    //Generating FORTH environment
    glossary.push_back(std::make_pair("CR", new Function(cr)));
    glossary.push_back(std::make_pair("SPACES", new Function(spaces)));
    glossary.push_back(std::make_pair("SPACE", new Function(space)));
    glossary.push_back(std::make_pair("EMIT", new Function(emit)));
    glossary.push_back(std::make_pair("+", new Function(add)));
    glossary.push_back(std::make_pair("-", new Function(sub)));
    glossary.push_back(std::make_pair("*", new Function(mult)));
    glossary.push_back(std::make_pair("UM*", new Function(umult)));
    glossary.push_back(std::make_pair("/", new Function(div)));
    glossary.push_back(std::make_pair("MOD", new Function(mod)));
    glossary.push_back(std::make_pair("/MOD", new Function(modDiv)));
    glossary.push_back(std::make_pair("*/", new Function(multDiv)));
    glossary.push_back(std::make_pair("*/MOD", new Function(multDivMod)));
    glossary.push_back(std::make_pair("1+", new Function(add1)));
    glossary.push_back(std::make_pair("1-", new Function(sub1)));
    glossary.push_back(std::make_pair("2+", new Function(add2)));
    glossary.push_back(std::make_pair("2-", new Function(sub2)));
    glossary.push_back(std::make_pair("2*", new Function(lshift)));
    glossary.push_back(std::make_pair("2/", new Function(rshift)));
    glossary.push_back(std::make_pair("AND", new Function(and_)));
    glossary.push_back(std::make_pair("OR", new Function(or_)));
    glossary.push_back(std::make_pair("ABS", new Function(abs)));
    glossary.push_back(std::make_pair("NEGATE", new Function(neg)));
    glossary.push_back(std::make_pair("MIN", new Function(min)));
    glossary.push_back(std::make_pair("MAX", new Function(max)));
    glossary.push_back(std::make_pair("SWAP", new Function(swap)));
    glossary.push_back(std::make_pair("2SWAP", new Function(swap2)));
    glossary.push_back(std::make_pair("DUP", new Function(dup)));
    glossary.push_back(std::make_pair("2DUP", new Function(dup2)));
    glossary.push_back(std::make_pair("?DUP", new Function(dup_if)));
    glossary.push_back(std::make_pair("OVER", new Function(over)));
    glossary.push_back(std::make_pair("2OVER", new Function(over2)));
    glossary.push_back(std::make_pair("ROT", new Function(rot)));
    glossary.push_back(std::make_pair("DROP", new Function(drop)));
    glossary.push_back(std::make_pair("DROP2", new Function(drop2)));
    glossary.push_back(std::make_pair(">R", new Function(retPush)));
    glossary.push_back(std::make_pair("R>", new Function(retPop)));
    glossary.push_back(std::make_pair("I", new Function(retCopy)));
    glossary.push_back(std::make_pair("R@", new Function(retCopy)));
    glossary.push_back(std::make_pair("J", new Function(retCopy3)));
    glossary.push_back(std::make_pair(".", new Function(print)));
    glossary.push_back(std::make_pair("U.", new Function(uprint)));
    glossary.push_back(std::make_pair("U.R", new Function(urjprint)));
    glossary.push_back(std::make_pair(".S", new Function(printS)));
    glossary.push_back(std::make_pair("=", new Function(equals)));
    glossary.push_back(std::make_pair("<", new Function(lessThan)));
    glossary.push_back(std::make_pair(">", new Function(greaterThan)));
    glossary.push_back(std::make_pair("0=", new Function(zeroEquals)));
    glossary.push_back(std::make_pair("0<", new Function(zeroLessThan)));
    glossary.push_back(std::make_pair("0>", new Function(zeroGreaterThan)));
    glossary.push_back(std::make_pair("PAGE", new Function(page)));
    glossary.push_back(std::make_pair("QUIT", new Function(quit)));
    glossary.push_back(std::make_pair("?STACK", new Function(stack_q)));

    while(!BYE) {
        printf("#F> ");
        ReadInput(stdin);
        try {
            text_interpreter(idx);
        } catch(int) {} //Abort catching
        free(buf);
        if(!BYE && !ABORT && !QUIT) printf(" ok\n");
        if(QUIT) QUIT = false;
        if(ABORT) ABORT = false;
        printf("\n");
    }
    //Destruction
    delete stack;
    delete return_stack;
    for(auto itr = glossary.begin(); itr != glossary.end(); itr++)
        delete itr->second;
    return 0;
}