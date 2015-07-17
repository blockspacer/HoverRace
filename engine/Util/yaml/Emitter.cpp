
// Emitter.cpp
//
// Copyright (c) 2008, 2009, 2014, 2015 Michael Imamura.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#include <boost/format.hpp>

#include "../Log.h"
#include "../Str.h"

#include "Emitter.h"

using namespace HoverRace::Util;

#define YAML_STR(s) ((yaml_char_t*)(s))

namespace HoverRace {
namespace Util {
namespace yaml {

namespace {

const std::string trueStr("true");
const std::string falseStr("false");

// Always use the "C" locale for writing numbers.
const std::locale stdLocale("C");
boost::format intFmt("%d", stdLocale);
boost::format floatFmt("%g", stdLocale);

const size_t MAX_STRING_LEN = 65536;

// Arbitrary limit on the maximum buffer we'll emit.
// This must be lower than numeric_limits<std::streamsize>::max().
const size_t MAX_OUTPUT_BUFFER_SIZE = 16 * 1024 * 1024;

}  // namespace

/**
 * Create a new emitter for a file handle.
 * @param file An open file for writing (may not be null).
 *             It is the caller's job to close the file when the emitter
 *             is destroyed.
 * @param versionDirective Include the @c YAML version directive at the start.
 */
Emitter::Emitter(FILE *file, bool versionDirective)
{
	InitEmitter();
	yaml_emitter_set_output_file(&emitter, file);
	InitStream(versionDirective);
}

/**
 * Create a new emitter for an output stream.
 * @param os The output stream.
 * @param versionDirective Include the @c YAML version directive at the start.
 */
Emitter::Emitter(std::ostream &os, bool versionDirective)
{
	InitEmitter();
	yaml_emitter_set_output(&emitter, &Emitter::OutputStreamHandler, &os);
	InitStream(versionDirective);
}

/**
 * Create a new emitter for a string.
 * @param s The output string.
 * @param versionDirective Include the @c YAML version directive at the start.
 */
Emitter::Emitter(std::string &s, bool versionDirective)
{
	InitEmitter();
	yaml_emitter_set_output(&emitter, &Emitter::OutputStringHandler, &s);
	InitStream(versionDirective);
}

/// Destructor.
Emitter::~Emitter()
{
	yaml_event_t event;

	// End the document.
	yaml_document_end_event_initialize(&event, 1);
	if(!yaml_emitter_emit(&emitter, &event)) {
		yaml_emitter_delete(&emitter);
		HR_LOG(error) << "Unable to end document.";
		return;
	}

	// End the stream.
	yaml_stream_end_event_initialize(&event);
	if(!yaml_emitter_emit(&emitter, &event)) {
		yaml_emitter_delete(&emitter);
		HR_LOG(error) << "Unable to end document.";
		return;
	}

	yaml_emitter_delete(&emitter);
}

/**
 * Initialize the LibYAML emitter.
 * This should only be called from the constructor.
 */
void Emitter::InitEmitter()
{
	memset(&emitter, 0, sizeof(emitter));
	yaml_emitter_initialize(&emitter);
}

/**
 * Initialize the stream and document.
 * This should only be called from the constructor.
 * @param versionDirective Include the @c YAML version directive at the start.
 */
void Emitter::InitStream(bool versionDirective)
{
	yaml_event_t event;

	// Start the stream.
	yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
	if(!yaml_emitter_emit(&emitter, &event))
		throw EmitterExn("Unable to start stream");

	yaml_version_directive_t version;
	version.major = 1;
	version.minor = 1;

	yaml_tag_directive_t nullTag;
	nullTag.handle = NULL;
	nullTag.prefix = NULL;

	// Start the document.
	yaml_document_start_event_initialize(&event,
		versionDirective ? &version : NULL,
		&nullTag, &nullTag, 1);
	if(!yaml_emitter_emit(&emitter, &event))
		throw EmitterExn("Unable to start document");
}

int Emitter::OutputStreamHandler(void *data, unsigned char *buffer, size_t size)
{
	std::ostream *os = (std::ostream*)data;

	if (size > MAX_OUTPUT_BUFFER_SIZE) {
		HR_LOG(warning) << "Output buffer size (" << size <<
			") exceeds maximum (" << MAX_OUTPUT_BUFFER_SIZE << "); truncating.";
		size = MAX_OUTPUT_BUFFER_SIZE;
	}

	os->write((const char*)buffer, static_cast<std::streamsize>(size));
	return 1;
}

int Emitter::OutputStringHandler(void *data, unsigned char *buffer, size_t size)
{
	std::string *s = (std::string*)data;

	if (size > MAX_OUTPUT_BUFFER_SIZE) {
		HR_LOG(warning) << "Output buffer size (" << size <<
			") exceeds maximum (" << MAX_OUTPUT_BUFFER_SIZE << "); truncating.";
		size = MAX_OUTPUT_BUFFER_SIZE;
	}

	s->append((const char*)buffer, size);
	return 1;
}

/**
 * Start a map.
 * Writing a map involves:
 * - First, call StartMap().
 * - For each key-value pair, first write the key with MapKey(),
 *   then write the value (e.g. Value(), another map, a sequence, etc.)
 * - Finally, call EndMap() to close the map.
 */
void Emitter::StartMap()
{
	yaml_event_t event;
	yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_MAPPING_STYLE);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to start map");
	}
}

/**
 * Write the key of a map.
 * @param s The key name (may not be empty).
 */
void Emitter::MapKey(const std::string &s)
{
	Value(s);
}

/// End the current map.
void Emitter::EndMap()
{
	yaml_event_t event;
	yaml_mapping_end_event_initialize(&event);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to end map");
	}
}

/**
 * Start a sequence.
 * Writing a sequence involves:
 * - First, call StartSeq().
 * - For each element: call Value() for a single scalar value, or add
 *   a map or a sub-sequence.
 * - Finally, call EndSeq() to close the sequence.
 */
void Emitter::StartSeq()
{
	yaml_event_t event;
	yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_BLOCK_SEQUENCE_STYLE);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to start sequence");
	}
}

/**
 * End the current sequence.
 */
void Emitter::EndSeq()
{
	yaml_event_t event;
	yaml_sequence_end_event_initialize(&event);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to end sequence");
	}
}

/**
 * Write a single string value.
 * @param val The value.
 */
void Emitter::Value(const std::string &val)
{
	int len;
	if (val.length() > MAX_STRING_LEN) {
		HR_LOG(warning) <<
			"Truncated YAML string "
			"(size is " << val.length() << ", "
			"max is " << MAX_STRING_LEN << "): \"" <<
			val.substr(0, 64) << "\"...";
		len = static_cast<int>(MAX_STRING_LEN);
	}
	else {
		len = static_cast<int>(val.length());
	}

	yaml_event_t event;
	yaml_scalar_event_initialize(&event, nullptr, nullptr,
		YAML_STR(val.c_str()), len, 1, 1, YAML_ANY_SCALAR_STYLE);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to write scalar value");
	}
}

/**
 * Write a single string value.
 * @param val The value (may not be @c nullptr).
 */
void Emitter::Value(const char *val)
{
	size_t slen = strlen(val);
	int len;
	if (slen > MAX_STRING_LEN) {
		std::string excerpt(val, 64);
		HR_LOG(warning) <<
			"Truncated YAML string "
			"(size is " << slen << ", "
			"max is " << MAX_STRING_LEN << "): \"" <<
			excerpt << "\"...";
		len = static_cast<int>(MAX_STRING_LEN);
	}
	else {
		len = static_cast<int>(slen);
	}

	yaml_event_t event;
	yaml_scalar_event_initialize(&event, nullptr, nullptr,
		YAML_STR(val), len, 1, 1, YAML_ANY_SCALAR_STYLE);
	if(!yaml_emitter_emit(&emitter, &event)) {
		throw EmitterExn("Unable to write scalar value");
	}
}

/**
 * Write a single boolean value.
 * @param val The value.
 */
void Emitter::Value(bool val)
{
	Value(val ? trueStr : falseStr);
}

/**
 * Write a single integer value.
 * @param val The value.
 */
void Emitter::Value(int val)
{
	Value(boost::str(intFmt % val));
}

/**
 * Write a single floating-point value.
 * @param val The value.
 */
void Emitter::Value(double val)
{
	Value(boost::str(floatFmt % val));
}

/**
 * Write a single floating-point value.
 * @param val The value.
 */
void Emitter::Value(float val)
{
	Value(static_cast<double>(val));
}

/**
 * Write a single path value.
 * @param path The value.
 */
void Emitter::Value(const HoverRace::Util::OS::path_t &path)
{
	Value(static_cast<const char*>(Str::PU(path)));
}

}  // namespace yaml
}  // namespace Util
}  // namespace HoverRace
