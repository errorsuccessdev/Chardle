/*
 * HOMEWORK:
 * - Try to find a worlde clone online that mimics wordle's
 *      duplicate behavior
 * - What is this hashmap thing chat won't shut up about ;)
 * 
 * NOW:
 * 
 * NEXT STREAM: 
 * Error handling when user enters > MAX_STRING_LENGTH characters
 * Maybe use this? https://en.cppreference.com/w/c/io/getchar 
 * 
 * FUTURE:
 * Nicer UI
 */

#define WIN32_LEAN_AND_MEAN

#pragma comment( lib, "bcrypt" )

#pragma once
#include "Chardle.h"
#include <stdio.h>
#include <assert.h>
#include <bcrypt.h>

#define COLOR_NORMAL    0
#define COLOR_GREEN     32
#define COLOR_YELLOW    33
#define COLOR_GREY      1000

int validateInput(char* input);
int checkInputAgainstAnswer(char* input, const char* answer);
void printText(char text, int color);
void printString(char* text, int color);
int isLetterInAnswer(char letter, char* tempAnswer, int* colors);
void clearScreen(void);
void printAlphabet(char letter, int color, int print);
int endGame(int won, char* answer);
int binarySearch(const char* guess, int start, int end);
void printBoard(const char* word, int* colors);
char* getRandomAnswer(void);

// RESEARCH: Is it bad that tthese are global?
char dictValid[NUM_VALID_WORDS][WORD_LENGTH + 1];
char dictAnswers[NUM_ANSWERS][WORD_LENGTH + 1];

int globalNumAnswers = NUM_ANSWERS;

int main()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    assert(hOut != INVALID_HANDLE_VALUE);
    DWORD consoleMode = 0;
    BOOL result = GetConsoleMode(
        hOut,
        &consoleMode
    );
    assert(result);
    DWORD originalConsoleMode = consoleMode;
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    result = SetConsoleMode(
        hOut, 
        consoleMode
    );
    assert(result);

    char buffer[MAX_STRING_LENGTH];
    int numCorrectLetters = 0;
    int numGuesses = 0;
    int gameStarted = FALSE;
    char* answer = 0;

    while (1)
    {
        if (NOT gameStarted)
        {
            answer = getRandomAnswer();
            gameStarted = TRUE;
        }
        printAlphabet(
            0,
            0, 
            TRUE
        );
        printf(
            "Guess a %d-letter word, or press q to quit: ",
            WORD_LENGTH
        );
        fgets(
            buffer,
            MAX_STRING_LENGTH,
            stdin
        );
        buffer[MAX_STRING_LENGTH - 1] = '\0';

        // How do we open stdin with fopen? Or can we?
        //FILE* stdinWritable = fopen(stdin, "w");
        //assert(stdinWritable);
        //int result = fflush(stdin);
        //assert(result == 0);

        // If the user has pressed 'q', quit immediately
        if (buffer[0] == 'q' && buffer[1] == '\n')
        {
            break;
        }

        int valid = validateInput(buffer);
        if (valid)
        {
            numCorrectLetters =
                checkInputAgainstAnswer(
                    buffer,
                    answer
                );
           numGuesses++;
        }

        // An end condition has been met
        if (numCorrectLetters == WORD_LENGTH || 
            numGuesses == NUM_GUESSES)
        {
            int won = (numCorrectLetters == WORD_LENGTH);
            if (endGame(
                won, 
                answer
            ))
            {
                break;
            }
            numCorrectLetters = 0;
            numGuesses = 0;
            gameStarted = FALSE;
        }
    }

    result = SetConsoleMode(
        hOut, 
        originalConsoleMode
    );
    assert(result);
}

char* getRandomAnswer(void)
{
    // Get random word
    unsigned int randomNumber = 0;
    NTSTATUS status = BCryptGenRandom(
        NULL,
        (unsigned char*) &randomNumber,
        sizeof(unsigned int),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    assert(status == 0);
    randomNumber %= globalNumAnswers;
    char* answer = dictAnswers[randomNumber];
    printf("%s\n", answer);
    return answer;
}

int binarySearch(const char* guess, int start, int end)
{
    // word isn't in the array
    if (start > end)
    {
        return -1;
    }
    int mid = end + (start - end) / 2;
    char* midWord = dictValid[mid];

    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        if (guess[index] < midWord[index])
        {
            return binarySearch(
                guess,
                start,
                mid - 1
            );
        }
        else if (guess[index] > midWord[index])
        {
            return binarySearch(
                guess,
                mid + 1,
                end
            );
        }
    }
    return mid;
}

int endGame(int won, char* answer)
{
    // Print game end message
    if (won)
    {
        printString(
            "Congratulations, you've won!\n",
            COLOR_GREEN
        );
    }
    else
    {
        printf(
            "Sorry, the word was %s.\n",
            answer
        );
    }

    // If they have exhausted the entire answer dictionary,
    //      quit out
    if (globalNumAnswers == 1)
    {
        printString(
            "\nYou have guessed all of the words in the game! "
            "Thanks for playing!\n",
            COLOR_GREEN
        );
        return TRUE;
    }

    // Ask the user if they would like to play again
    char buffer[MAX_STRING_LENGTH] = { 0 };
    printf(
        "Press any key to play again, or q to quit: "
    );
    fgets(
        buffer,
        MAX_STRING_LENGTH,
        stdin
    );
    buffer[MAX_STRING_LENGTH - 1] = '\0';
    if (buffer[0] == 'q' && buffer[1] == '\n')
    {
        return TRUE;
    }

    // If they want to continue, reset the game
    for (char letter = 'a';
         letter <= 'z';
         letter++)
    {
        printAlphabet(
            letter, 
            COLOR_NORMAL, 
            FALSE
        );
    }

    // Move remaining words up in the array
    int numCharsPerElement = WORD_LENGTH + 1;
    int answerIndex = (int)
        ((answer - dictAnswers[0]) /
        numCharsPerElement);
    int numAnswersLeft = globalNumAnswers - answerIndex;
    int numCharsLeft = 
        numAnswersLeft * numCharsPerElement;

    // Asaf you are a legend!
    void* result = memmove(
        answer,
        answer + (WORD_LENGTH + 1),
        numCharsLeft
    );
    assert(result);
    globalNumAnswers--;

    clearScreen();
    return FALSE;
}

// If they pass 0 as the letter parameter, skip updates
void printAlphabet(char letter, int color, int print)
{
    static char alphabet[26] = { 0 };
    static int colors[26] = { 0 };

    // Initialize alphabet if necessary
    if (alphabet[0] == 0)
    {
        for (char letter = 'a';
             letter <= 'z';
             letter++)
        {
            alphabet[letter - 'a'] = letter;
        }
    }

    // Update letter color if desired
    if (letter >= 'a' &&
        letter <= 'z')
    {
        colors[letter - 'a'] = color;
    }

    if (print)
    {
        for (int index = 0;
             index < 26;
             index++)
        {
            printText(
                alphabet[index], 
                colors[index]
            );
            printf(" ");
        }
        printf("\n");
    }
}

void clearScreen(void)
{
    wchar sequences[3][7] = {
        L"\x1b[2J",
        L"\x1b[3J",
        L"\x1b[0;0H"
    };

    for (int index = 0;
         index < 3;
         index++)
    {
        wchar* sequence = sequences[index];
        DWORD sequenceLength = (DWORD) wcslen(sequence);
        DWORD written = 0;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        BOOL result = WriteConsoleW(
            hStdOut,
            sequence,
            sequenceLength,
            &written,
            NULL
        );
        assert(result);
        assert(written == sequenceLength);
    }
}

void printString(char* text, int color)
{
    // We have to use custom RGB to get grey
    if (color == COLOR_GREY)
    {
        printf("\x1b[38;2;103;103;103m%s",
               text);
    }

    // Otherwise, we can use the system color codes
    else
    {
        printf(
            "\x1b[%dm%s",
            color,
            text
        );
    }

    printf(
        "\x1b[%dm", 
        COLOR_NORMAL
    );
}

void printText(char text, int color)
{
    // We have to use custom RGB to get grey
    if (color == COLOR_GREY)
    {
        printf("\x1b[38;2;103;103;103m%c",
               text);
    }

    // Otherwise, we can use the system color codes
    else
    {
        printf(
            "\x1b[%dm%c",
            color,
            text
        );
    }

    printf(
        "\x1b[%dm",
        COLOR_NORMAL
    );
}

// TODO: This could certainly be refactored into something nicer
int checkInputAgainstAnswer(char* input, const char* answer)
{
    int correctLetters = 0;
    int colors[WORD_LENGTH] = { 0 };

    // Check for exactly correct letters
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = input[index];

        // Is letter in the right spot?
        if (letter == answer[index])
        {
            printAlphabet(
                letter, 
                COLOR_GREEN,
                FALSE
            );
            colors[index] = COLOR_GREEN;
            correctLetters++;
        }
    }

    // Copy answer into tempAnswer so we can modify it
    char tempAnswer[WORD_LENGTH + 1] = { 0 };
    for (int index = 0;
         index < (WORD_LENGTH + 1);
         index++)
    {
        tempAnswer[index] = answer[index];
    }

    // Check for correct letters in incorrect spots
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = input[index];
        if (colors[index] != COLOR_GREEN)
        {
            // Is letter in the word at all?
            if (isLetterInAnswer(
                letter,
                tempAnswer,
                colors
            ))
            {
                printAlphabet(
                    letter,
                    COLOR_YELLOW,
                    FALSE
                );
                colors[index] = COLOR_YELLOW;
            }
        }
    }

    // Copy answer into tempAnswer to reset tempAnswer
    for (int index = 0;
         index < (WORD_LENGTH + 1);
         index++)
    {
        tempAnswer[index] = answer[index];
    }

    // Set any letters not in the word to the color grey
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        int color = colors[index];
        char letter = input[index];

        if (color != COLOR_GREEN &&
            color != COLOR_YELLOW)
        {
            colors[index] = COLOR_GREY;
            printAlphabet(
                letter,
                COLOR_GREY,
                FALSE
            );
        }
    }

    printBoard(input, colors);

    return correctLetters;
}

void printBoard(const char* word, int* colors)
{
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        printText(
            word[index],
            colors[index]
        );
    }
    printf("\n");
}

int isLetterInAnswer(
    char letter,
    char* tempAnswer,
    int* colors
)
{
    for (int index = 0;
         tempAnswer[index] != '\0';
         index++)
    {
        if (colors[index] != COLOR_GREEN)
        {
            if (letter == tempAnswer[index])
            {
                tempAnswer[index] = '_';
                return TRUE;
            }
        }
    }
    return FALSE;
}

int validateInput(char* input)
{
    // Make sure input is 5 characters long
    int length;
    for (length = 0;
         input[length] != '\0';
         length++)
    { };
    if (length != 6)
    {
        return FALSE;
    }

    // Make sure input only contains letters
    for (int index = 0;
         index < (length - 1); // Avoid newline character
         index++)
    {
        char letter = input[index];

        // Convert from uppercase if necessary
        if (letter >= 'A' &&
            letter <= 'Z')
        {
            letter += LOWERCASE_OFFSET;
            input[index] = letter;
        }

        // Verify letter is lowercase
        if (letter < 'a' || letter > 'z')
        {
            return FALSE;
        }
    }

    // Make sure it is in our dictionary
    int isInDictionary = binarySearch(
        input, 
        0, 
        (NUM_VALID_WORDS - 1)
    );
    if (isInDictionary == -1)
    {
        printf("%s not in dictionary!\n", input);
        return FALSE;
    }
    return TRUE;
}