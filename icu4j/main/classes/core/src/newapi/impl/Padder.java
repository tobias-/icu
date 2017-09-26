// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package newapi.impl;

import com.ibm.icu.impl.number.Modifier;
import com.ibm.icu.impl.number.NumberStringBuilder;

public class Padder {
    public static final String FALLBACK_PADDING_STRING = "\u0020"; // i.e. a space

    public enum PadPosition {
      BEFORE_PREFIX,
      AFTER_PREFIX,
      BEFORE_SUFFIX,
      AFTER_SUFFIX;

      public static PadPosition fromOld(int old) {
        switch (old) {
          case com.ibm.icu.text.DecimalFormat.PAD_BEFORE_PREFIX:
            return PadPosition.BEFORE_PREFIX;
          case com.ibm.icu.text.DecimalFormat.PAD_AFTER_PREFIX:
            return PadPosition.AFTER_PREFIX;
          case com.ibm.icu.text.DecimalFormat.PAD_BEFORE_SUFFIX:
            return PadPosition.BEFORE_SUFFIX;
          case com.ibm.icu.text.DecimalFormat.PAD_AFTER_SUFFIX:
            return PadPosition.AFTER_SUFFIX;
          default:
            throw new IllegalArgumentException("Don't know how to map " + old);
        }
      }

      public int toOld() {
        switch (this) {
          case BEFORE_PREFIX:
            return com.ibm.icu.text.DecimalFormat.PAD_BEFORE_PREFIX;
          case AFTER_PREFIX:
            return com.ibm.icu.text.DecimalFormat.PAD_AFTER_PREFIX;
          case BEFORE_SUFFIX:
            return com.ibm.icu.text.DecimalFormat.PAD_BEFORE_SUFFIX;
          case AFTER_SUFFIX:
            return com.ibm.icu.text.DecimalFormat.PAD_AFTER_SUFFIX;
          default:
            return -1; // silence compiler errors
        }
      }
    }

    private static final Padder NONE = new Padder(null, -1, null);

    String paddingString;
    int targetWidth;
    PadPosition position;

    public Padder(String paddingString, int targetWidth, PadPosition position) {
        // TODO: Add a few default instances
        this.paddingString = (paddingString == null) ? " " : paddingString;
        this.targetWidth = targetWidth;
        this.position = (position == null) ? PadPosition.BEFORE_PREFIX : position;
    }

    public static Padder none() {
        return NONE;
    }

    public static Padder codePoints(int cp, int targetWidth, PadPosition position) {
        // TODO: Validate the code point
        if (targetWidth >= 0) {
            String paddingString = String.valueOf(Character.toChars(cp));
            return new Padder(paddingString, targetWidth, position);
        } else {
            throw new IllegalArgumentException("Padding width must not be negative");
        }
    }

    public boolean isValid() {
        return targetWidth > 0;
    }

    public int padAndApply(Modifier mod1, Modifier mod2, NumberStringBuilder string, int leftIndex, int rightIndex) {
        int modLength = mod1.getCodePointCount() + mod2.getCodePointCount();
        int requiredPadding = targetWidth - modLength - string.codePointCount();
        assert leftIndex == 0 && rightIndex == string.length(); // fix the previous line to remove this assertion

        int length = 0;
        if (requiredPadding <= 0) {
            // Padding is not required.
            length += mod1.apply(string, leftIndex, rightIndex);
            length += mod2.apply(string, leftIndex, rightIndex + length);
            return length;
        }

        if (position == PadPosition.AFTER_PREFIX) {
            length += addPaddingHelper(paddingString, requiredPadding, string, leftIndex);
        } else if (position == PadPosition.BEFORE_SUFFIX) {
            length += addPaddingHelper(paddingString, requiredPadding, string, rightIndex + length);
        }
        length += mod1.apply(string, leftIndex, rightIndex + length);
        length += mod2.apply(string, leftIndex, rightIndex + length);
        if (position == PadPosition.BEFORE_PREFIX) {
            length += addPaddingHelper(paddingString, requiredPadding, string, leftIndex);
        } else if (position == PadPosition.AFTER_SUFFIX) {
            length += addPaddingHelper(paddingString, requiredPadding, string, rightIndex + length);
        }

        // The length might not be exactly right due to currency spacing.
        // Make an adjustment if needed.
        while (string.codePointCount() < targetWidth) {
            int insertIndex = mod1.getPrefixLength() + mod2.getPrefixLength();
            switch (position) {
            case AFTER_PREFIX:
                insertIndex += leftIndex;
                break;
            case BEFORE_SUFFIX:
                insertIndex += rightIndex;
                break;
            default:
                // Should not happen since currency spacing is always on the inside.
                throw new AssertionError();
            }
            addPaddingHelper(paddingString, requiredPadding, string, insertIndex);
        }

        return length;
    }

    private static int addPaddingHelper(String paddingString, int requiredPadding, NumberStringBuilder string,
            int index) {
        for (int i = 0; i < requiredPadding; i++) {
            // TODO: If appending to the end, this will cause actual insertion operations. Improve.
            string.insert(index, paddingString, null);
        }
        return paddingString.length() * requiredPadding;
    }
}