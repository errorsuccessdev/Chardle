/*
 * HOMEWORK:
 * Look into how to do typedefs
 *
 * NEXT STREAM:
 * When letter is correct, keep it
 *   green even if the user moves it
 * 
 * FUTURE:
 * Handle duplicate letters
 * Implement dictionary
 * Check if input is a real word
 * Nicer UI
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

#define COLOR_NORMAL        0
#define COLOR_GREEN         32
#define COLOR_YELLOW        33

int validateInput(char* input);
int checkInputAgainstAnswer(char* input, char* answer);
void printText(char text, int color);
void printTextArray(char* text, int color);
int isLetterInWord(letter, answer);
void clearScreen(void);
void printAlphabet(char letter, int color, int print);
int endGame(int won, char* answer);

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
        printAlphabet(0, 0, TRUE);
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
        }
    }

    result = SetConsoleMode(
        hOut, 
        originalConsoleMode
    );
    assert(result);
}

int endGame(int won, char* answer)
{
    // Print game end message
    if (won)
    {
        printTextArray(
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
        printAlphabet(letter, COLOR_NORMAL, FALSE);
    }
    clearScreen();
    return FALSE;
}

// This is probably stupid
// IF they pass NULL as the letter parameter, skip updates
void printAlphabet(char letter, int color, int print)
{
    static char alphabet[26] = { 0 };
    static int colors[26] = { COLOR_NORMAL };

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
            printText(alphabet[index], colors[index]);
            printf(" ");
        }
        printf("\n");
    }
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
        COLOR_NORMAL
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
        COLOR_NORMAL
    );
}

int checkInputAgainstAnswer(char* input, char* answer)
{
    int correctLetters = 0;
    int colors[WORD_LENGTH] = { COLOR_NORMAL };

    // Iterate over the input
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

        // Is letter in the word at all?
        else if (isLetterInWord(
            letter,
            answer
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