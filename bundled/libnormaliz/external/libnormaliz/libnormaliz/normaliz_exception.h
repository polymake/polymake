/*
 * Normaliz
 * Copyright (C) 2007-2022  W. Bruns, B. Ichim, Ch. Soeger, U. v. d. Ohe
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * As an exception, when this program is distributed through (i) the App Store
 * by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or (iii) Google Play
 * by Google Inc., then that store may impose any digital rights management,
 * device limits and/or redistribution restrictions that are required by its
 * terms of service.
 */

#ifndef LIBNORMALIZ_NORMALIZ_EXEPTION_H_
#define LIBNORMALIZ_NORMALIZ_EXEPTION_H_

#include <exception>
#include <string>
#include <sstream>
#include <libnormaliz/general.h>
#include <libnormaliz/cone_property.h>

namespace libnormaliz {

class NormalizException : public std::exception {
   public:
    virtual const char* what() const noexcept = 0;
};

class ArithmeticException : public NormalizException {
   public:
    ArithmeticException()
        : msg("Overflow detected. A fatal size excess or a computation overflow.\n If Normaliz has terminated and you are using "
              "LongLong, rerun without it.") {
    }
    ~ArithmeticException() noexcept {
    }

    template <typename Integer>
    ArithmeticException(const Integer& convert_number) {
        static int CCCCCCC = 0;

        CCCCCCC++;
        /* if(CCCCCCC>=3)
         assert(false);*/
        std::stringstream stream;
        stream << "Could not convert " << convert_number << ".\n";
        stream << "Overflow detected. A fatal size excess or a computation overflow.\n If Normaliz has terminated and you are "
                  "using LongLong, rerun without it.";
        msg = stream.str();
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class NonpointedException : public NormalizException {
   public:
    virtual const char* what() const noexcept {
        return "Cone is not pointed.";
    }
};

class NotIntegrallyClosedException : public NormalizException {
   public:
    virtual const char* what() const noexcept {
        return "Original monoid is not integrally closed.";
    }
};

class BadInputException : public NormalizException {
   public:
    BadInputException(const std::string& message) : msg("Some error in the normaliz input data detected: " + message) {
    }
    ~BadInputException() noexcept {
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class NmzCoCoAException : public NormalizException {
   public:
    NmzCoCoAException(const std::string& message) : msg(message) {
    }
    ~NmzCoCoAException() noexcept {
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class NotComputableException : public NormalizException {
   public:
    NotComputableException(const std::string& message) : msg("Could not compute: " + message) {
    }
    NotComputableException(const ConeProperties& missing) {
        std::stringstream stream;
        stream << "Could not compute: " << missing << "!";
        msg = stream.str();
    }
    ~NotComputableException() noexcept {
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class FatalException : public NormalizException {
   public:
    FatalException(const std::string& message)
        : msg("Fatal error: " + message + "\nThis should not happen, please contact the developers!") {
    }
    ~FatalException() noexcept {
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class InterruptException : public NormalizException {
   public:
    InterruptException(const std::string& message) : msg("Interrupted: " + message) {
    }
    ~InterruptException() noexcept {
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class NumberFieldInputException : public NormalizException {
   public:
    virtual const char* what() const noexcept {
        return "Input requested a number field, which is not available in this version.";
    }
};

class LongException : public NormalizException {
   public:
    ~LongException() noexcept {
    }

    template <typename Integer>
    LongException(const Integer& convert_number) {
        std::stringstream stream;
        stream << "Could not convert " << convert_number << "to Long.\n";
        stream << "The number would break an absolute size barrier.";
        msg = stream.str();
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class LongLongException : public NormalizException {
   public:
    ~LongLongException() noexcept {
    }

    template <typename Integer>
    LongLongException(const Integer& convert_number) {
        std::stringstream stream;
        stream << "Could not convert " << convert_number << "to Long long.\n";
        stream << "The number would break an absolute size barrier.";
        msg = stream.str();
    }

    virtual const char* what() const noexcept {
        return msg.c_str();
    }

   private:
    std::string msg;
};

class PredictionErrorException : public NormalizException {
   public:
    virtual const char* what() const noexcept {
        return "Wrong prediction by floating point.";
    }
};

}  // namespace libnormaliz

#endif /* LIBNORMALIZ_H_ */
