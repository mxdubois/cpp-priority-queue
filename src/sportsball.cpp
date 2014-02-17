#include <iostream>
using std::cout;
using std::cin;
using std::endl;
#include <iomanip>
using std::setw;
using std::setfill;
#include <string>
using std::string;
using std::stoi;
#include <stdexcept>
using std::invalid_argument;
using std::out_of_range;
#include <cstring>
using std::strlen;
#include <sstream>
using std::stringstream;
using std::istringstream;
#include <fstream>
using std::getline;
#include <chrono>
using std::chrono::time_point;
using std::chrono::duration;
using std::chrono::steady_clock;
#include <exception>
using std::exception;
#include <memory>
using std::shared_ptr;

#include "PriorityQueue.hpp"

/**
 * Functions for running sportsball
 */
namespace sportsball {

static const bool DEBUG = false;
static const int MILLIS_PER_SECOND = 1000;

static const string SUB_PLAYER_TOKEN = "GO!";
static const char INLINE_DELIMITER = '/';
static const string TITLE = "SPORTSBALL!";

/**
 * Returns this programs help string.
 *
 * @param programName - name to display in `Usage: programName...etc`
 */
string helpstr(string programName) {
	return "Usage: " + programName + " dataFile [initialSize] [stepSize]\n" +
			"mandatory arguments: \n"
			"\n\tdataFile - string, path to a data file wherein each line " +
			" contains a space-separated pair of connected node ids" +
			"\noptional arguments:" +
			"\n\tinitialCapacity - int, number of elements the queue should" +
			"should support before the first resize." +
			"\n\tstepSize - int, number of elements by which to increase the" +
			"size of the queue when the allocated size is exceeded.";
}

int playBall(string dataFile, size_t initialCapacity, size_t stepSize) {
	int returnVal = 1;  // pessimism to boot

	// File input based on example here:
	// http://stackoverflow.com/a/7868998
	std::ifstream infile(dataFile);

	// Fail if we can't open the file.
	if(!infile.is_open()) {
		cout << "File could not be opened." << endl;
		returnVal = 1;
	} else {
		int pad = 80 - TITLE.size() - 5;
		cout << setfill('#') << setw(4) << " "
				<< TITLE
				<< " " << setfill('#') <<  setw(pad) << "#" << endl;

		PriorityQueue<shared_ptr<string> >
			playerQueue(initialCapacity, stepSize);
		string line, priorityString;
		int priority = 0;
		int lineNumber = 0;
		istringstream lineStream;

		try {

			// For each line in the file
			while (getline(infile, line)) {
				if(line == SUB_PLAYER_TOKEN) {
					// If there is a player to poll
					if(!playerQueue.empty()) {
						// Print their name
						shared_ptr<string> pName = playerQueue.top();
						cout << *pName << " enters the game." << endl;
						playerQueue.pop();
						//pName.reset(); // discard our pointer to the string
					} else {
						cout << "No one is ready!" << endl;
					}
				} else {
					lineStream.str(line); // replace the current string
					lineStream.clear(); // reset flags

					std::shared_ptr<std::string> pName(new string);

					// We can use getLine to parse up to our delimiter
					getline(lineStream, *pName, INLINE_DELIMITER);
					// Then extract the rest of the line normally
					getline(lineStream, priorityString);

					// Attempt to convert `priorityString` to an int
					// (we catch exceptions further down)
					priority = stoi(priorityString);

					if(sportsball::DEBUG) {
						cout << "Inserting " << *pName << "/"
							<< priority << " (@" << pName << ")" << endl;
					}

					// Cool. That worked. Now queue the player.
					playerQueue.insert(pName, priority);
					pName.reset();
					priorityString.clear();
				}

				if(sportsball::DEBUG) {
					cout << "size: " << playerQueue.getSize() << "; "
							<< "capacity: "
							<< playerQueue.getCapacity() << "; "
							<< "numResizes: "
							<< playerQueue.getNumResizes() << "." << endl;
				}

				lineNumber++; // track line number
			}

			cout << setfill('-') << setw(80) << "-" <<endl;
			cout << "At the end, there were " << playerQueue.getSize()
					<< " players left." << endl;
			cout << "The array was resized " << playerQueue.getNumResizes()
					<< " times." << endl;

			returnVal = 0;

		} catch(invalid_argument& e) {
			cout << "There was a problem reading in the priority"
					<< " on line " << (lineNumber + 1) << "." << endl;
			cout << e.what();
		} catch (out_of_range& e) {
			cout << "There was a problem reading in the priority"
					<< " on line " << (lineNumber + 1) << "." << endl;
			cout << e.what();
		}
		infile.close();

	}
	return returnVal;
}

} /* End namespace sportsball*/

/**
 * Global, main entry-point.
 */
int main(int argc, const char* argv[]) {
	// Begin tracking elapsed time
	// We'll use the C++11 additions for this because it's easiest.
	// See: http://en.cppreference.com/w/cpp/chrono
	time_point<steady_clock> start, end;
	start = steady_clock::now();

	int returnVal = 1; // assume error will occur

	const int requiredArgs = 1;
	const int optionalArgs = 2;
	const int maxArgs = 1 + requiredArgs + optionalArgs;
	const int minArgs = 1 + requiredArgs;
	const string programName= string(argv[0]);

	// If we only get the program name
	if(argc == 1) {
		// Print usage without error
		cout << sportsball::helpstr(programName) << endl;
		returnVal =  1;

	} else if(argc >= minArgs && argc <= maxArgs ) {
		// We got the right number of args
		string dataFile = (argv[1]) ? string(argv[1]) : string();

		// Defaults
		size_t initialCapacity = PriorityQueueBase::DEFAULT_INITIAL_CAPACITY;
		size_t stepSize = PriorityQueueBase::DEFAULT_STEP_SIZE;

		try {

		// Override defaults with user input.
		// Although the priority queue uses size_t, we limit user input
		// to the int range for this implementation
		if(argc >= 3) {
			// Convert c-string to integer
			initialCapacity = stoi(argv[2]);
		}
		if(argc >= 4) {
			// Convert c-string to integer
			stepSize = stoi(argv[3]);
		}

		// Run the game and capture result
		returnVal = sportsball::playBall(dataFile, initialCapacity, stepSize);
		} catch(invalid_argument& e) {
			cout << "You entered a non-integer value for an integer parameter."
					<< endl;
			cout << e.what();
		} catch (out_of_range& e) {
			cout << "You entered a value outside the `int` range for "
					<< " an integer parameter."<< endl;
			cout << e.what();
		}

	} else {
		// We got too many or too few args
		cout << "Invalid arguments." << endl
			 << sportsball::helpstr(programName) << endl;
		returnVal = 1;
	}

	// Capture end time
	end = steady_clock::now();

	// Compute and print elapsed
	duration<double> elapsed = end-start;
	double elapsedMillis = elapsed.count() * sportsball::MILLIS_PER_SECOND;
	cout << "Elapsed " <<  elapsedMillis << "ms." << endl;

	return returnVal;
}
