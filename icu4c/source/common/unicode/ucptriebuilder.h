// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

// ucptriebuilder.h (split out of ucptrie.h)
// created: 2018jan24 Markus W. Scherer

#ifndef __UCPTRIEBUILDER_H__
#define __UCPTRIEBUILDER_H__

#include "unicode/utypes.h"
#include "unicode/localpointer.h"
#include "unicode/ucptrie.h"
#include "unicode/utf8.h"
#include "putilimp.h"
#include "udataswp.h"

U_CDECL_BEGIN

/**
 * \file
 *
 * TODO
 *
 * The following function and macros are used for highly optimized UTF-16
 * text processing. The UCPTRIE_FAST_U16_NEXTxy() macros do not depend on these.
 *
 * UTF-16 text processing can be optimized by detecting surrogate pairs and
 * assembling supplementary code points only when there is non-trivial data
 * available.
 *
 * At build-time, use ucptriebld_getRange() starting from U+10000 to see if there
 * is non-trivial (non-initialValue) data for any of the supplementary
 * code points associated with a lead surrogate.
 * If so, then set a special (application-specific) value for the
 * lead surrogate.
 *
 * At runtime, use UCPTRIE_FAST_BMP_GET16() or
 * UCPTRIE_FAST_BMP_GET32() per code unit. If there is non-trivial
 * data and the code unit is a lead surrogate, then check if a trail surrogate
 * follows. If so, assemble the supplementary code point with
 * U16_GET_SUPPLEMENTARY() and look up its value with UCPTRIE_FAST_SUPP_GET16()
 * or UCPTRIE_FAST_SUPP_GET32(); otherwise deal with the unpaired surrogate in some way.
 *
 * If there is only trivial data for lead and trail surrogates, then processing
 * can often skip them. For example, in normalization or case mapping
 * all characters that do not have any mappings are simply copied as is.
 */

/**
 * Opaque trie builder structure.
 * @see UCPTrie
 */
struct UCPTrieBuilder;
typedef struct UCPTrieBuilder UCPTrieBuilder;

/**
 * Creates an empty trie builder. At build time, 32-bit data values are used.
 * ucptrie_freeze() takes a valueBits parameter
 * which determines the data value width in the serialized and frozen forms.
 * You must ucptriebld_close() the trie builder once you are done using it.
 *
 * @param initialValue the initial value that is set for all code points
 * @param errorValue the value for out-of-range code points and ill-formed UTF-8/16
 * @param pErrorCode an in/out ICU UErrorCode
 * @return a pointer to the allocated and initialized new builder
 */
U_CAPI UCPTrieBuilder * U_EXPORT2
ucptriebld_open(uint32_t initialValue, uint32_t errorValue, UErrorCode *pErrorCode);

/**
 * Clones a trie builder.
 * You must ucptrie_close() the clone once you are done using it.
 *
 * @param other the trie builder to clone
 * @param pErrorCode an in/out ICU UErrorCode
 * @return a pointer to the new trie builder clone
 */
U_CAPI UCPTrieBuilder * U_EXPORT2
ucptriebld_clone(const UCPTrieBuilder *other, UErrorCode *pErrorCode);

/**
 * Closes a trie builder and release associated memory.
 *
 * @param builder the builder
 */
U_CAPI void U_EXPORT2
ucptriebld_close(UCPTrieBuilder *builder);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN

/**
 * \class LocalUCPTrieBuilderPointer
 * "Smart pointer" class, closes a UCPTrieBuilder via ucptriebld_close().
 * For most methods see the LocalPointerBase base class.
 *
 * @see LocalPointerBase
 * @see LocalPointer
 * @draft ICU 62
 */
U_DEFINE_LOCAL_OPEN_POINTER(LocalUCPTrieBuilderPointer, UCPTrieBuilder, ucptriebld_close);

U_NAMESPACE_END

#endif

/**
 * Creates a trie builder with the same contents as the input trie.
 * You must ucptriebld_close() the builder once you are done using it.
 *
 * @param trie the trie to clone
 * @param pErrorCode an in/out ICU UErrorCode
 * @return a pointer to the new trie builder
 */
U_CAPI UCPTrieBuilder * U_EXPORT2
ucptriebld_fromUCPTrie(const UCPTrie *trie, UErrorCode *pErrorCode);

/**
 * Get a value from a code point as stored in the trie builder.
 *
 * @param builder the trie builder
 * @param c the code point
 * @return the value
 */
U_CAPI uint32_t U_EXPORT2
ucptriebld_get(const UCPTrieBuilder *builder, UChar32 c);

/**
 * Returns the last code point such that all those from start to there have the same value.
 * Can be used to efficiently iterate over all same-value ranges in a trie builder.
 *
 * For each entry in the trie builder, the value to be delivered is passed through
 * the UCPTrieHandleValue function.
 * The value is unchanged if that function pointer is NULL.
 *
 * See the same-signature ucptrie_getRange() for a code sample.
 *
 * @param builder a pointer to a trie builder
 * @param start range start
 * @param handleValue a pointer to a function that may modify the trie builder entry value,
 *     or NULL if the values from the trie builder are to be used directly
 * @param context an opaque pointer that is passed on to the handleValue function
 * @param pValue if not NULL, receives the value that every code point start..end has;
 *     optionally modified by handleValue(context, builder value)
 * @return the range end code point, or -1 if start is not a valid code point
 */
U_CAPI UChar32 U_EXPORT2
ucptriebld_getRange(const UCPTrieBuilder *builder, UChar32 start,
                    UCPTrieHandleValue *handleValue, const void *context, uint32_t *pValue);

/**
 * Set a value for a code point.
 *
 * @param builder the unfrozen trie builder
 * @param c the code point
 * @param value the value
 * @param pErrorCode an in/out ICU UErrorCode; among other possible error codes:
 * - U_NO_WRITE_PERMISSION if the trie builder is frozen
 */
U_CAPI void U_EXPORT2
ucptriebld_set(UCPTrieBuilder *builder, UChar32 c, uint32_t value, UErrorCode *pErrorCode);

/**
 * Set a value in a range of code points [start..end].
 * All code points c with start<=c<=end will get the value if
 * overwrite is TRUE or if the old value is the initial value.
 *
 * @param builder the unfrozen trie builder
 * @param start the first code point to get the value
 * @param end the last code point to get the value (inclusive)
 * @param value the value
 * @param pErrorCode an in/out ICU UErrorCode; among other possible error codes:
 * - U_NO_WRITE_PERMISSION if the trie builder is frozen
 */
U_CAPI void U_EXPORT2
ucptriebld_setRange(UCPTrieBuilder *builder,
                    UChar32 start, UChar32 end,
                    uint32_t value, UErrorCode *pErrorCode);

/**
 * Freeze a trie builder. Make it immutable (read-only) and compact it,  // TODO: doc as _build()
 * ready for serialization and for use with fast macros.
 * Functions to set values will fail after serializing.
 *
 * A trie builder can be frozen only once. If this function is called again with different
 * valueBits then it will set a U_ILLEGAL_ARGUMENT_ERROR.
 *
 * @param builder the trie builder
 * @param valueBits selects the data entry size; if smaller than 32 bits, then
 *                  the values stored in the trie builder will be truncated
 * @param pErrorCode an in/out ICU UErrorCode; among other possible error codes:
 * - U_INDEX_OUTOFBOUNDS_ERROR if the compacted index or data arrays are too long
 *                             for serialization
 *                             (the trie builder will be immutable and usable,
 *                             but not frozen and not usable with the fast macros)
 *
 * @see ucptriebld_fromUCPTrie
 */
U_CAPI UCPTrie * U_EXPORT2
ucptriebld_build(UCPTrieBuilder *builder, UCPTrieType type, UCPTrieValueBits valueBits,
                 UErrorCode *pErrorCode);

#ifdef UCPTRIE_DEBUG
U_CFUNC void
ucptrie_printLengths(const UCPTrie *trie, const char *which);

U_CFUNC void ucptriebld_setName(UCPTrieBuilder *builder, const char *name);
#endif

U_CDECL_END

#endif
