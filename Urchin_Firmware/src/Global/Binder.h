//
// Created by gabri on 6/4/26.
//

#ifndef URCHIN_FRAMEWORK_2_BINDER_H
#define URCHIN_FRAMEWORK_2_BINDER_H




#define ContextWordSize 15
#define NumOfActions 8
#define MaxVPIDs 256


    typedef struct Context Context;  // Forward declaration

/**
 *@brief A context is a function to be called when a word is seen in message from the PI
 */
typedef struct Context{
    /**
     * The word or phrase that will signal a kind of action
     */
    const char Name[ContextWordSize];

    /**
     * The length of Name
     */
    const unsigned short int depth;

    /**
     * The function to be called when the name was mentioned
     * @param VPID The VPID this function was called from
     * @param buffer[] The remaining string after the "Name"
     */
    int (*function)(unsigned char VPID ,const char buffer[]);

    /**
     * list contexts to branch to
     */
    Context* branch;

    /**
     * Number of contexts in list
     */
    const unsigned int size;

}Context;


extern Context (*Conversation[MaxVPIDs])[NumOfActions];
extern int ConversationLength;
extern Context* Identifier;


int BinderInit();

#endif //URCHIN_FRAMEWORK_2_BINDER_H