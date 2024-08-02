/*
 * Handle duplicate letters!
 * Look into how to do typedefs
 * Optimization of win/lose code
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <Windows.h>

#define NOT     !
#define TRUE    1
#define FALSE   0

#define MAX_STRING_LENGTH   500
#define GAME_MAX_NUMBER     10
#define WORD_LENGTH         5
#define LOWERCASE_OFFSET    32
#define NUM_GUESSES         6

#define TEXT_NORMAL         0
#define TEXT_GREEN          32
#define TEXT_YELLOW         33

int validateInput(char* input);
int checkInputAgainstAnswer(char* input, char* answer);
void printText(char text, int color);
void printTextArray(char* text, int color);
int isLetterInWord(letter, answer);
void clearScreen(void);

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

    //srand((int) time(0));
    //int answer = (rand() % GAME_MAX_NUMBER) + 1;

    char* answer = "coder";

    char buffer[MAX_STRING_LENGTH];

    int numCorrectLetters = 0;
    int numGuesses = 0;
    while (1)
    {
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

        // They won
        if (numCorrectLetters == WORD_LENGTH)
        {
            printTextArray(
                "Congratulations, you've won!\n",
                TEXT_GREEN
            );
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
                break;
            }
            clearScreen();
        }

        // They lost
        else if (numGuesses == NUM_GUESSES)
        {
            printf(
                "Sorry, the word was %s.\n",
                answer
            );
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
                break;
            }
            clearScreen();
        }
    }

    result = SetConsoleMode(
        hOut, 
        originalConsoleMode
    );
    assert(result);
}

void clearScreen(void)
{
    wchar_t sequences[3][7] = {
        L"\x1b[2J",
        L"\x1b[3J",
        L"\x1b[0;0H"
    };

    for (int index = 0;
         index < 3;
         index++)
    {
        wchar_t* sequence = sequences[index];
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

void printTextArray(char* text, int color)
{
    printf(
        "\x1b[%dm%s", 
        color, 
        text
    );
    printf(
        "\x1b[%dm", 
        TEXT_NORMAL
    );
}

void printText(char text, int color)
{
    printf(
        "\x1b[%dm%c", 
        color, 
        text
    );
    printf(
        "\x1b[%dm", 
        TEXT_NORMAL
    );
}

int checkInputAgainstAnswer(char* input, char* answer)
{
    int correctLetters = 0;
    int colors[WORD_LENGTH] = { TEXT_NORMAL };

    // Iterate over the input
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = input[index];

        // Is letter in the right spot?
        if (letter == answer[index])
        {
            colors[index] = TEXT_GREEN;
            correctLetters++;
        }

        // Is letter in the word at all?
        else if (isLetterInWord(
            letter,
            answer
        ))
        {
            colors[index] = TEXT_YELLOW;
        }
    }

    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        printText(
            input[index], 
            colors[index]
        );
    }
    printf("\n");
    return correctLetters;
}

// RESEARCH: Should this be unsigned char?
int isLetterInWord(
    char letter,
    char* word
)
{
    for (int index = 0;
         word[index] != '\0';
         index++)
    {
        if (letter == word[index])
        {
            return TRUE;
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
         length++);
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
    return TRUE;
}