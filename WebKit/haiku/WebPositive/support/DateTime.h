/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATE_TIME_H_
#define _DATE_TIME_H_


#include <String.h>


class BMessage;


namespace BPrivate {


enum time_type {
	B_GMT_TIME,
	B_LOCAL_TIME
};


enum diff_type {
	B_HOURS_DIFF,
	B_MINUTES_DIFF,
	B_SECONDS_DIFF,
	B_MILLISECONDS_DIFF,
	B_MICROSECONDS_DIFF
};


class BTime {
public:
								BTime();
								BTime(const BTime& other);
								BTime(int32 hour, int32 minute, int32 second,
									int32 microsecond = 0);
								BTime(const BMessage* archive);
								~BTime();

			status_t			Archive(BMessage* into) const;

			bool				IsValid() const;
			bool				IsValid(const BTime& time) const;
			bool				IsValid(int32 hour, int32 minute, int32 second,
									int32 microsecond = 0) const;

	static	BTime				CurrentTime(time_type type);

			BTime				Time() const;
			bool				SetTime(const BTime& time);
			bool				SetTime(int32 hour, int32 minute, int32 second,
									int32 microsecond = 0);

			BTime&				AddHours(int32 hours);
			BTime&				AddMinutes(int32 minutes);
			BTime&				AddSeconds(int32 seconds);
			BTime&				AddMilliseconds(int32 milliseconds);
			BTime&				AddMicroseconds(int32 microseconds);

			int32				Hour() const;
			int32				Minute() const;
			int32				Second() const;
			int32				Millisecond() const;
			int32				Microsecond() const;
			bigtime_t			Difference(const BTime& time,
									diff_type type) const;

			bool				operator!=(const BTime& time) const;
			bool				operator==(const BTime& time) const;

			bool				operator<(const BTime& time) const;
			bool				operator<=(const BTime& time) const;

			bool				operator>(const BTime& time) const;
			bool				operator>=(const BTime& time) const;

private:
			bigtime_t			_Microseconds() const;
			BTime&				_AddMicroseconds(bigtime_t microseconds);
			bool				_SetTime(bigtime_t hour, bigtime_t minute,
									bigtime_t second, bigtime_t microsecond);

private:
			bigtime_t			fMicroseconds;
};


class BDate {
public:
								BDate();
								BDate(const BDate& other);
								BDate(int32 year, int32 month, int32 day);
								BDate(const BMessage* archive);
								~BDate();

			status_t			Archive(BMessage* into) const;

			bool				IsValid() const;
			bool				IsValid(const BDate& date) const;
			bool				IsValid(int32 year, int32 month,
									int32 day) const;

	static	BDate				CurrentDate(time_type type);

			BDate				Date() const;
			bool				SetDate(const BDate& date);

			bool				SetDate(int32 year, int32 month, int32 day);
			void				GetDate(int32* year, int32* month, int32* day);

			void				AddDays(int32 days);
			void				AddYears(int32 years);
			void				AddMonths(int32 months);

			int32				Day() const;
			int32				Year() const;
			int32				Month() const;
			int32				Difference(const BDate& date) const;

			int32				DayOfWeek() const;
			int32				DayOfYear() const;

			int32				WeekNumber() const;
			bool				IsLeapYear(int32 year) const;

			int32				DaysInYear() const;
			int32				DaysInMonth() const;

			BString				ShortDayName() const;
	static	BString				ShortDayName(int32 day);

			BString				ShortMonthName() const;
	static	BString				ShortMonthName(int32 month);

			BString				LongDayName() const;
	static	BString				LongDayName(int32 day);

			BString				LongMonthName() const;
	static	BString				LongMonthName(int32 month);

			int32				DateToJulianDay() const;
	static	BDate				JulianDayToDate(int32 julianDay);

			bool				operator!=(const BDate& date) const;
			bool				operator==(const BDate& date) const;

			bool				operator<(const BDate& date) const;
			bool				operator<=(const BDate& date) const;

			bool				operator>(const BDate& date) const;
			bool				operator>=(const BDate& date) const;

private:
			int32				_DaysInMonth(int32 year, int32 month) const;
			bool				_SetDate(int32 year, int32 month, int32 day);
			int32				_DateToJulianDay(int32 year, int32 month,
									int32 day) const;

private:
			int32				fDay;
			int32				fYear;
			int32				fMonth;
};


class BDateTime {
public:
								BDateTime();
								BDateTime(const BDate &date, const BTime &time);
								BDateTime(const BMessage* archive);
								~BDateTime();

			status_t			Archive(BMessage* into) const;

			bool				IsValid() const;

	static	BDateTime			CurrentDateTime(time_type type);
			void				SetDateTime(const BDate &date, const BTime &time);

			BDate&				Date();
			const BDate&		Date() const;
			void				SetDate(const BDate &date);

			BTime&				Time();
			const BTime&		Time() const;
			void				SetTime(const BTime &time);

			int32				Time_t() const;
			void				SetTime_t(uint32 seconds);

			bool				operator!=(const BDateTime& dateTime) const;
			bool				operator==(const BDateTime& dateTime) const;

			bool				operator<(const BDateTime& dateTime) const;
			bool				operator<=(const BDateTime& dateTime) const;

			bool				operator>(const BDateTime& dateTime) const;
			bool				operator>=(const BDateTime& dateTime) const;

private:
			BDate				fDate;
			BTime				fTime;
};


}	// namespace BPrivate


using BPrivate::time_type;
using BPrivate::B_GMT_TIME;
using BPrivate::B_LOCAL_TIME;
using BPrivate::diff_type;
using BPrivate::B_HOURS_DIFF;
using BPrivate::B_MINUTES_DIFF;
using BPrivate::B_SECONDS_DIFF;
using BPrivate::B_MILLISECONDS_DIFF;
using BPrivate::B_MICROSECONDS_DIFF;
using BPrivate::BTime;
using BPrivate::BDate;
using BPrivate::BDateTime;


#endif	// _DATE_TIME_H_
