/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun <host.haiku@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "DateTime.h"


#include <time.h>
#include <sys/time.h>

#include <Message.h>


namespace BPrivate {


const int32			kSecondsPerMinute			= 60;

const int32			kHoursPerDay				= 24;
const int32			kMinutesPerDay				= 1440;
const int32			kSecondsPerDay				= 86400;
const int32			kMillisecondsPerDay			= 86400000;

const bigtime_t		kMicrosecondsPerSecond		= 1000000LL;
const bigtime_t		kMicrosecondsPerMinute		= 60000000LL;
const bigtime_t		kMicrosecondsPerHour		= 3600000000LL;
const bigtime_t		kMicrosecondsPerDay			= 86400000000LL;


/*!
	Constructs a new BTime object. Asked for its time representation, it will
	return 0 for Hour(), Minute(), Second() etc. This can represent midnight,
	but be aware IsValid() will return false.
*/
BTime::BTime()
	: fMicroseconds(-1)
{
}


/*!
	Constructs a BTime object with \c hour \c minute, \c second, \c microsecond.

	\c hour must be between 0 and 23, \c minute and \c second must be between
	0 and 59 and \c microsecond should be in the range of 0 and 999999. If the
	specified time is invalid, the time is not set and IsValid() returns false.
*/
BTime::BTime(int32 hour, int32 minute, int32 second, int32 microsecond)
	: fMicroseconds(-1)
{
	_SetTime(hour, minute, second, microsecond);
}


/*!
	Constructs a new BTime object from the provided BMessage archive.
*/
BTime::BTime(const BMessage* archive)
	: fMicroseconds(-1)
{
	if (archive == NULL)
		return;
	archive->FindInt64("mircoseconds", &fMicroseconds);
}


/*!
	Empty destructor.
*/
BTime::~BTime()
{
}


/*!
	Archives the BTime object into the provided BMessage object.
	@returns	\c B_OK if all went well.
				\c B_BAD_VALUE, if the message is \c NULL.
				\c other error codes, depending on failure to append
				fields to the message.
*/
status_t
BTime::Archive(BMessage* into) const
{
	if (into == NULL)
		return B_BAD_VALUE;
	return into->AddInt64("mircoseconds", fMicroseconds);
}


/*!
	Returns true if the time is valid, otherwise false. A valid time can be
	BTime(23, 59, 59, 999999) while BTime(24, 00, 01) would be invalid.
*/
bool
BTime::IsValid() const
{
	return fMicroseconds > -1 && fMicroseconds < kMicrosecondsPerDay;
}


/*!
	This is an overloaded member function, provided for convenience.
*/
bool
BTime::IsValid(const BTime& time) const
{
	return time.fMicroseconds > -1 && time.fMicroseconds < kMicrosecondsPerDay;
}


/*!
	This is an overloaded member function, provided for convenience.
*/
bool
BTime::IsValid(int32 hour, int32 minute, int32 second, int32 microsecond) const
{
	return BTime(hour, minute, second, microsecond).IsValid();
}


/*!
	Returns the current time as reported by the system depending on the given
	time_type \c type.
*/
BTime
BTime::CurrentTime(time_type type)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
		// gettimeofday failed?
		time(&tv.tv_sec);
	}

	struct tm result;
	struct tm* timeinfo;
	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&tv.tv_sec, &result);
	else
		timeinfo = localtime_r(&tv.tv_sec, &result);

	int32 sec = timeinfo->tm_sec;
	return BTime(timeinfo->tm_hour, timeinfo->tm_min, (sec > 59) ? 59 : sec,
		tv.tv_usec);
}


/*!
	Returns a copy of the current BTime object.
*/
BTime
BTime::Time() const
{
	return *this;
}


/*!
	This is an overloaded member function, provided for convenience. Set the
	current BTime object to the passed BTime \c time object.
*/
bool
BTime::SetTime(const BTime& time)
{
	fMicroseconds = time.fMicroseconds;
	return IsValid();
}


/*!
	Set the time to \c hour \c minute, \c second and \c microsecond.

	\c hour must be between 0 and 23, \c minute and \c second must be between
	0 and 59 and \c microsecond should be in the range of 0 and 999999. Returns
	true if the time is valid; otherwise false. If the specified time is
	invalid, the time is not set and the function returns false.
*/
bool
BTime::SetTime(int32 hour, int32 minute, int32 second, int32 microsecond)
{
	return _SetTime(hour, minute, second, microsecond);
}



/*!
	Adds \c hours to the current time. If the passed value is negativ it will
	become earlier. Note: The time will wrap if it passes midnight.
*/
void
BTime::AddHours(int32 hours)
{
	_AddMicroseconds(bigtime_t(hours % kHoursPerDay) * kMicrosecondsPerHour);
}


/*!
	Adds \c minutes to the current time. If the passed value is negativ it will
	become earlier. Note: The time will wrap if it passes midnight.
*/
void
BTime::AddMinutes(int32 minutes)
{
	_AddMicroseconds(bigtime_t(minutes % kMinutesPerDay) *
		kMicrosecondsPerMinute);
}


/*!
	Adds \c seconds to the current time. If the passed value is negativ it will
	become earlier. Note: The time will wrap if it passes midnight.
*/
void
BTime::AddSeconds(int32 seconds)
{
	_AddMicroseconds(bigtime_t(seconds % kSecondsPerDay) *
		kMicrosecondsPerSecond);
}


/*!
	Adds \c milliseconds to the current time. If the passed value is negativ it
	will become earlier. Note: The time will wrap if it passes midnight.
*/
void
BTime::AddMilliseconds(int32 milliseconds)
{
	_AddMicroseconds(bigtime_t(milliseconds % kMillisecondsPerDay) * 1000);
}


/*!
	Adds \c microseconds to the current time. If the passed value is negativ it
	will become earlier. Note: The time will wrap if it passes midnight.
*/
void
BTime::AddMicroseconds(int32 microseconds)
{
	_AddMicroseconds(microseconds);
}


/*!
	Returns the hour fragment of the time.
*/
int32
BTime::Hour() const
{
	return int32(_Microseconds() / kMicrosecondsPerHour);
}


/*!
	Returns the minute fragment of the time.
*/
int32
BTime::Minute() const
{
	return int32(((_Microseconds() % kMicrosecondsPerHour)) / kMicrosecondsPerMinute);
}


/*!
	Returns the second fragment of the time.
*/
int32
BTime::Second() const
{
	return int32(_Microseconds() / kMicrosecondsPerSecond) % kSecondsPerMinute;
}


/*!
	Returns the millisecond fragment of the time.
*/
int32
BTime::Millisecond() const
{

	return Microsecond() / 1000;
}


/*!
	Returns the microsecond fragment of the time.
*/
int32
BTime::Microsecond() const
{
	return int32(_Microseconds() % 1000000);
}


bigtime_t
BTime::_Microseconds() const
{
	return fMicroseconds == -1 ? 0 : fMicroseconds;
}


/*!
	Returns the difference between this time and the given BTime \c time based
	on the passed diff_type \c type. If \c time is earlier the return value will
	be negativ.

	The return value then can be hours, minutes, seconds, milliseconds or
	microseconds while its range will always be between -86400000000 and
	86400000000 depending on diff_type \c type.
*/
bigtime_t
BTime::Difference(const BTime& time, diff_type type) const
{
	bigtime_t diff = time._Microseconds() - _Microseconds();
	switch (type) {
		case B_HOURS_DIFF: {
			diff /= kMicrosecondsPerHour;
		}	break;
		case B_MINUTES_DIFF: {
			diff /= kMicrosecondsPerMinute;
		}	break;
		case B_SECONDS_DIFF: {
			diff /= kMicrosecondsPerSecond;
		}	break;
		case B_MILLISECONDS_DIFF: {
			diff /= 1000;
		}	break;
		case B_MICROSECONDS_DIFF:
		default:	break;
	}
	return diff;
}


/*!
	Returns true if this time is different from \c time, otherwise false.
*/
bool
BTime::operator!=(const BTime& time) const
{
	return fMicroseconds != time.fMicroseconds;
}


/*!
	Returns true if this time is equal to \c time, otherwise false.
*/
bool
BTime::operator==(const BTime& time) const
{
	return fMicroseconds == time.fMicroseconds;
}


/*!
	Returns true if this time is earlier than \c time, otherwise false.
*/
bool
BTime::operator<(const BTime& time) const
{
	return fMicroseconds < time.fMicroseconds;
}


/*!
	Returns true if this time is earlier than or equal to \c time, otherwise false.
*/
bool
BTime::operator<=(const BTime& time) const
{
	return fMicroseconds <= time.fMicroseconds;
}


/*!
	Returns true if this time is later than \c time, otherwise false.
*/
bool
BTime::operator>(const BTime& time) const
{
	return fMicroseconds > time.fMicroseconds;
}


/*!
	Returns true if this time is later than or equal to \c time, otherwise false.
*/
bool
BTime::operator>=(const BTime& time) const
{
	return fMicroseconds >= time.fMicroseconds;
}


void
BTime::_AddMicroseconds(bigtime_t microseconds)
{
	bigtime_t count = 0;
	if (microseconds < 0) {
		count = ((kMicrosecondsPerDay - microseconds) / kMicrosecondsPerDay) *
			kMicrosecondsPerDay;
	}
	fMicroseconds = (_Microseconds() + microseconds + count) % kMicrosecondsPerDay;
}


bool
BTime::_SetTime(bigtime_t hour, bigtime_t minute, bigtime_t second,
	bigtime_t microsecond)
{
	fMicroseconds = hour * kMicrosecondsPerHour +
					minute * kMicrosecondsPerMinute +
					second * kMicrosecondsPerSecond +
					microsecond;

	bool isValid = IsValid();
	if (!isValid)
		fMicroseconds = -1;

	return isValid;
}


//	#pragma mark - BDate


/*!
	Constructs a new BDate object. IsValid() will return false.
*/
BDate::BDate()
	: fDay(-1),
	  fYear(0),
	  fMonth(-1)
{
}


/*!
	Constructs a BDate object with \c year \c month and \c day.

	Please note that a date before 1.1.4713 BC, a date with year 0 and a date
	between 4.10.1582 and 15.10.1582 are considered invalid. If the specified
	date is invalid, the date is not set and IsValid() returns false. Also note
	that every passed year will be interpreted as is.

*/
BDate::BDate(int32 year, int32 month, int32 day)
{
	_SetDate(year, month, day);
}


/*!
	Constructs a new BDate object from the provided archive.
*/
BDate::BDate(const BMessage* archive)
	: fDay(-1),
	  fYear(0),
	  fMonth(-1)
{
	if (archive == NULL)
		return;
	archive->FindInt32("day", &fDay);
	archive->FindInt32("year", &fYear);
	archive->FindInt32("month", &fMonth);
}


/*!
	Empty destructor.
*/
BDate::~BDate()
{
}


/*!
	Archives the BDate object into the provided BMessage object.
	@returns	\c B_OK if all went well.
				\c B_BAD_VALUE, if the message is \c NULL.
				\c other error codes, depending on failure to append
				fields to the message.
*/
status_t
BDate::Archive(BMessage* into) const
{
	if (into == NULL)
		return B_BAD_VALUE;
	status_t ret = into->AddInt32("day", fDay);
	if (ret == B_OK)
		ret = into->AddInt32("year", fYear);
	if (ret == B_OK)
		ret = into->AddInt32("month", fMonth);
	return ret;
}


/*!
	Returns true if the date is valid, otherwise false.

	Please note that a date before 1.1.4713 BC, a date with year 0 and a date
	between 4.10.1582 and 15.10.1582 are considered invalid.
*/
bool
BDate::IsValid() const
{
	return IsValid(fYear, fMonth, fDay);
}


/*!
	This is an overloaded member function, provided for convenience.
*/
bool
BDate::IsValid(const BDate& date) const
{
	return IsValid(date.fYear, date.fMonth, date.fDay);
}


/*!
	This is an overloaded member function, provided for convenience.
*/
bool
BDate::IsValid(int32 year, int32 month, int32 day) const
{
	// no year 0 in Julian and nothing before 1.1.4713 BC
	if (year == 0 || year < -4713)
		return false;

	if (month < 1 || month > 12)
		return false;

	if (day < 1 || day > _DaysInMonth(year, month))
		return false;

	// 'missing' days between switch julian - gregorian
	if (year == 1582 && month == 10 && day > 4 && day < 15)
		return false;

	return true;
}


/*!
	Returns the current date as reported by the system depending on the given
	time_type \c type.
*/
BDate
BDate::CurrentDate(time_type type)
{
	time_t timer;
	struct tm result;
	struct tm* timeinfo;

	time(&timer);

	if (type == B_GMT_TIME)
		timeinfo = gmtime_r(&timer, &result);
	else
		timeinfo = localtime_r(&timer, &result);

	return BDate(timeinfo->tm_year + 1900, timeinfo->tm_mon +1, timeinfo->tm_mday);
}


/*!
	Returns a copy of the current BTime object.
*/
BDate
BDate::Date() const
{
	return *this;
}


/*!
	This is an overloaded member function, provided for convenience.
*/
bool
BDate::SetDate(const BDate& date)
{
	return _SetDate(date.fYear, date.fMonth, date.fDay);
}


/*!
	Set the date to \c year \c month and \c day.

	Returns true if the date is valid; otherwise false. If the specified date is
	invalid, the date is not set and the function returns false.
*/
bool
BDate::SetDate(int32 year, int32 month, int32 day)
{
	return _SetDate(year, month, day);
}


/*!
	This function sets the given \c year, \c month and \c day to the
	representative values of this date. The pointers can be NULL. If the date is
	invalid, the values will be set to -1 for \c month and \c day, the \c year
	will be set to 0.
*/
void
BDate::GetDate(int32* year, int32* month, int32* day)
{
	if (year)
		*year = fYear;

	if (month)
		*month = fMonth;

	if (day)
		*day = fDay;
}


/*!
	Adds \c days to the current date. If the passed value is negativ it will
	become earlier. If the current date is invalid, the \c days are not added.
*/
void
BDate::AddDays(int32 days)
{
	if (IsValid())
		*this = JulianDayToDate(DateToJulianDay() + days);
}


/*!
	Adds \c years to the current date. If the passed value is negativ it will
	become earlier. If the current date is invalid, the \c years are not added.
	The day/ month combination will be adjusted if it does not exist in the
	resulting year, so this function will then return the latest valid date.
*/
void
BDate::AddYears(int32 years)
{
	if (IsValid()) {
		const int32 tmp = fYear;
		fYear += years;

		if ((tmp > 0 && fYear <= 0) || (tmp < 0 && fYear >= 0))
			fYear += (years > 0) ? +1 : -1;

		fDay = min_c(fDay, _DaysInMonth(fYear, fMonth));
	}
}


/*!
	Adds \c months to the current date. If the passed value is negativ it will
	become earlier. If the current date is invalid, the \c months are not added.
	The day/ month combination will be adjusted if it does not exist in the
	resulting year, so this function will then return the latest valid date.
*/
void
BDate::AddMonths(int32 months)
{
	if (IsValid()) {
		const int32 tmp = fYear;
		fYear += months / 12;
		fMonth +=  months % 12;

		if (fMonth > 12) {
			fYear++;
			fMonth -= 12;
		} else if (fMonth < 1) {
			fYear--;
			fMonth += 12;
		}

		if ((tmp > 0 && fYear <= 0) || (tmp < 0 && fYear >= 0))
			fYear += (months > 0) ? +1 : -1;

		// 'missing' days between switch julian - gregorian
		if (fYear == 1582 && fMonth == 10 && fDay > 4 && fDay < 15)
			fDay = (months > 0) ? 15 : 4;

		fDay = min_c(fDay, DaysInMonth());
	}
}


/*!
	Returns the day fragment of the date. The return value will be in the range
	of 1 to 31, in case the date is invalid it will be -1.
*/
int32
BDate::Day() const
{
	return fDay;
}


/*!
	Returns the year fragment of the date. If the date is invalid, the function
	returns 0.
*/
int32
BDate::Year() const
{
	return fYear;
}


/*!
	Returns the month fragment of the date. The return value will be in the
	range of 1 to 12, in case the date is invalid it will be -1.
*/
int32
BDate::Month() const
{
	return fMonth;
}


/*!
	Returns the difference in days between this date and the given BDate \c date.
	If \c date is earlier the return value will be negativ. If the calculation
	is done with an invalid date, the result is undefined.
*/
int32
BDate::Difference(const BDate& date) const
{
	return date.DateToJulianDay() - DateToJulianDay();
}


/*!
	Returns the week number of the date, if the date is invalid it will return
	B_ERROR. Please note that this function does only work within the Gregorian
	calendar, thus a date before 15.10.1582 will return B_ERROR.
*/
int32
BDate::WeekNumber() const
{
	/*
		This algorithm is taken from:
		Frequently Asked Questions about Calendars
		Version 2.8 Claus Tøndering 15 December 2005

		Note: it will work only within the Gregorian Calendar
	*/

	if (!IsValid() || fYear < 1582
		|| (fYear == 1582 && fMonth < 10)
		|| (fYear == 1582 && fMonth == 10 && fDay < 15))
		return int32(B_ERROR);

	int32 a;
	int32 b;
	int32 s;
	int32 e;
	int32 f;

	if (fMonth > 0 && fMonth < 3) {
		a = fYear - 1;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = 0;
		f = fDay - 1 + 31 * (fMonth - 1);
	} else if (fMonth >= 3 && fMonth <= 12) {
		a = fYear;
		b = (a / 4) - (a / 100) + (a / 400);
		int32 c = ((a - 1) / 4) - ((a - 1) / 100) + ((a -1) / 400);
		s = b - c;
		e = s + 1;
		f = fDay + ((153 * (fMonth - 3) + 2) / 5) + 58 + s;
	} else
		return int32(B_ERROR);

	int32 g = (a + b) % 7;
	int32 d = (f + g - e) % 7;
	int32 n = f + 3 - d;

	int32 weekNumber;
	if (n < 0)
		weekNumber = 53 - (g -s) / 5;
	else if (n > 364 + s)
		weekNumber = 1;
	else
		weekNumber = n / 7 + 1;

	return weekNumber;
}


/*!
	Returns the day of the week in the range of 1 to 7, while 1 stands for
	monday. If the date is invalid, the function will return B_ERROR.
*/
int32
BDate::DayOfWeek() const
{
	// http://en.wikipedia.org/wiki/Julian_day#Calculation
	return IsValid() ? (DateToJulianDay() % 7) + 1 : int32(B_ERROR);
}


/*!
	Returns the day of the year in the range of 1 to 365 (366 in leap years). If
	the date is invalid, the function will return B_ERROR.
*/
int32
BDate::DayOfYear() const
{
	if (!IsValid())
		return int32(B_ERROR);

	return DateToJulianDay() - _DateToJulianDay(fYear, 1, 1) + 1;
}


/*!
	Returns true if the passed \c year is a leap year, otherwise false. If the
	\c year passed is before 4713 BC, the result is undefined.
*/
bool
BDate::IsLeapYear(int32 year) const
{
	if (year < 1582) {
		if (year < 0)
			year++;
		return (year % 4) == 0;
	}
	return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}


/*!
	Returns the number of days in the year of the current date. If the date is
	valid it will return 365 or 366, otherwise B_ERROR;
*/
int32
BDate::DaysInYear() const
{
	if (!IsValid())
		return int32(B_ERROR);

	return IsLeapYear(fYear) ? 366 : 365;
}


/*!
	Returns the number of days in the month of the current date. If the date is
	valid it will return 28 up to 31, otherwise B_ERROR;
*/
int32
BDate::DaysInMonth() const
{
	if (!IsValid())
		return int32(B_ERROR);

	return _DaysInMonth(fYear, fMonth);
}


/*!
	Returns the short day name in case of an valid day, otherwise an empty
	string. The passed \c day must be in the range of 1 to 7 while 1 stands for
	monday.
*/
BString
BDate::ShortDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%a", &tm_struct);

	return BString(buffer);
}


/*!
	Returns the short month name in case of an valid month, otherwise an empty
	string. The passed \c month must be in the range of 1 to 12.
*/
BString
BDate::ShortMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month - 1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%b", &tm_struct);

	return BString(buffer);
}


/*!
	Returns the long day name in case of an valid day, otherwise an empty
	string. The passed \c day must be in the range of 1 to 7 while 1 stands for
	monday.
*/
BString
BDate::LongDayName(int32 day) const
{
	if (day < 1 || day > 7)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_wday = day == 7 ? 0 : day;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%A", &tm_struct);

	return BString(buffer);
}


/*!
	Returns the long month name in case of an valid month, otherwise an empty
	string. The passed \c month must be in the range of 1 to 12.
*/
BString
BDate::LongMonthName(int32 month) const
{
	if (month < 1 || month > 12)
		return BString();

	tm tm_struct;
	memset(&tm_struct, 0, sizeof(tm));
	tm_struct.tm_mon = month - 1;

	char buffer[256];
	strftime(buffer, sizeof(buffer), "%B", &tm_struct);

	return BString(buffer);
}


/*!
	Converts the date to Julian day. If your date is invalid, the function will
	return B_ERROR.
*/
int32
BDate::DateToJulianDay() const
{
	return _DateToJulianDay(fYear, fMonth, fDay);
}


/*
	Converts the passed \c julianDay to an BDate. If the \c julianDay is negativ,
	the function will return an invalid date. Because of the switch from Julian
	calendar to Gregorian calendar the 4.10.1582 is followed by the 15.10.1582.
*/
BDate
BDate::JulianDayToDate(int32 julianDay)
{
	BDate date;
	const int32 kGregorianCalendarStart = 2299161;
	if (julianDay >= kGregorianCalendarStart) {
		// http://en.wikipedia.org/wiki/Julian_day#Gregorian_calendar_from_Julian_day_number
		int32 j = julianDay + 32044;
		int32 dg = j % 146097;
		int32 c = (dg / 36524 + 1) * 3 / 4;
		int32 dc = dg - c * 36524;
		int32 db = dc % 1461;
		int32 a = (db / 365 + 1) * 3 / 4;
		int32 da = db - a * 365;
		int32 m = (da * 5 + 308) / 153 - 2;
		date.fYear = ((j / 146097) * 400 + c * 100 + (dc / 1461) * 4 + a) - 4800 +
			(m + 2) / 12;
		date.fMonth = (m + 2) % 12 + 1;
		date.fDay = int32((da - (m + 4) * 153 / 5 + 122) + 1.5);
	} else if (julianDay >= 0) {
		// http://en.wikipedia.org/wiki/Julian_day#Calculation
		julianDay += 32082;
		int32 d = (4 * julianDay + 3) / 1461;
		int32 e = julianDay - (1461 * d) / 4;
		int32 m = ((5 * e) + 2) / 153;
		date.fDay = e - (153 * m + 2) / 5 + 1;
		date.fMonth = m + 3 - 12 * (m / 10);
		int32 year = d - 4800 + (m / 10);
		if (year <= 0)
			year--;
		date.fYear = year;
	}
	return date;
}


/*!
	Returns true if this date is different from \c date, otherwise false.
*/
bool
BDate::operator!=(const BDate& date) const
{
	return DateToJulianDay() != date.DateToJulianDay();
}


/*!
	Returns true if this date is equal to \c date, otherwise false.
*/
bool
BDate::operator==(const BDate& date) const
{
	return DateToJulianDay() == date.DateToJulianDay();
}


/*!
	Returns true if this date is earlier than \c date, otherwise false.
*/
bool
BDate::operator<(const BDate& date) const
{
	return DateToJulianDay() < date.DateToJulianDay();
}


/*!
	Returns true if this date is earlier than or equal to \c date, otherwise false.
*/
bool
BDate::operator<=(const BDate& date) const
{
	return DateToJulianDay() <= date.DateToJulianDay();
}


/*!
	Returns true if this date is later than \c date, otherwise false.
*/
bool
BDate::operator>(const BDate& date) const
{
	return DateToJulianDay() > date.DateToJulianDay();
}


/*!
	Returns true if this date is later than or equal to \c date, otherwise false.
*/
bool
BDate::operator>=(const BDate& date) const
{
	return DateToJulianDay() >= date.DateToJulianDay();
}


bool
BDate::_SetDate(int32 year, int32 month, int32 day)
{
	fDay = -1;
	fYear = 0;
	fMonth = -1;

	bool valid = IsValid(year, month, day);
	if (valid) {
		fDay = day;
		fYear = year;
		fMonth = month;
	}

	return valid;
}


int32
BDate::_DaysInMonth(int32 year, int32 month) const
{
	if (month == 2 && IsLeapYear(year))
		return 29;

	const int32 daysInMonth[12] =
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	return daysInMonth[month -1];
}


int32
BDate::_DateToJulianDay(int32 _year, int32 month, int32 day) const
{
	if (IsValid(_year, month, day)) {
		int32 year = _year;
		if (year < 0) year++;

		int32 a = (14 - month) / 12;
		int32 y = year + 4800 - a;
		int32 m = month + (12 * a) - 3;

		// http://en.wikipedia.org/wiki/Julian_day#Calculation
		if (year > 1582
			|| (year == 1582 && month > 10)
			|| (year == 1582 && month == 10 && day >= 15)) {
			return day + (((153 * m) + 2) / 5) + (365 * y) + (y / 4) -
				(y / 100) + (y / 400) - 32045;
		} else if (year < 1582
			|| (year == 1582 && month < 10)
			|| (year == 1582 && month == 10 && day <= 4)) {
			return day + (((153 * m) + 2) / 5) + (365 * y) + (y / 4) - 32083;
		}
	}

	// http://en.wikipedia.org/wiki/Gregorian_calendar:
	//		The last day of the Julian calendar was Thursday October 4, 1582
	//		and this was followed by the first day of the Gregorian calendar,
	//		Friday October 15, 1582 (the cycle of weekdays was not affected).
	return int32(B_ERROR);
}


//	#pragma mark - BDateTime


/*!
	Constructs a new BDateTime object. IsValid() will return false.
*/
BDateTime::BDateTime()
	: fDate(),
	  fTime()
{
}


/*!
	Constructs a BDateTime object with \c date and \c time. The return value
	of IsValid() depends on the validity of the passed objects.
*/
BDateTime::BDateTime(const BDate& date, const BTime& time)
	: fDate(date),
	  fTime(time)
{
}


/*!
	Constructs a new BDateTime object. IsValid() will return false.
*/
BDateTime::BDateTime(const BMessage* archive)
	: fDate(archive),
	  fTime(archive)
{
}


/*!
	Empty destructor.
*/
BDateTime::~BDateTime()
{
}


/*!
	Archives the BDateTime object into the provided BMessage object.
	@returns	\c B_OK if all went well.
				\c B_BAD_VALUE, if the message is \c NULL.
				\c other error codes, depending on failure to append
				fields to the message.
*/
status_t
BDateTime::Archive(BMessage* into) const
{
	status_t ret = fDate.Archive(into);
	if (ret == B_OK)
		ret = fTime.Archive(into);
	return ret;
}


/*!
	Returns true if the date time is valid, otherwise false.
*/
bool
BDateTime::IsValid() const
{
	return fDate.IsValid() && fTime.IsValid();
}


/*!
	Returns the current date and time as reported by the system depending on the
	given time_type \c type.
*/
BDateTime
BDateTime::CurrentDateTime(time_type type)
{
	return BDateTime(BDate::CurrentDate(type), BTime::CurrentTime(type));
}


/*!
	Sets the current date and time of this object to \c date and \c time.
*/
void
BDateTime::SetDateTime(const BDate& date, const BTime& time)
{
	fDate = date;
	fTime = time;
}


/*!
	Returns the current date of this object.
*/
BDate
BDateTime::Date() const
{
	return fDate;
}


/*!
	Set the current date of this object to \c date.
*/
void
BDateTime::SetDate(const BDate& date)
{
	fDate = date;
}


/*!
	Returns the current time of this object.
*/
BTime
BDateTime::Time() const
{
	return fTime;
}


/*!
	Sets the current time of this object to \c time.
*/
void
BDateTime::SetTime(const BTime& time)
{
	fTime = time;
}


/*!
	Returns the current date and time converted to seconds since
	1.1.1970 - 00:00:00. If the current date is before 1.1.1970 the function
	returns -1;
*/
int32
BDateTime::Time_t() const
{
	BDate date(1970, 1, 1);
	if (date.Difference(fDate) < 0)
		return -1;

	tm tm_struct;

	tm_struct.tm_hour = fTime.Hour();
	tm_struct.tm_min = fTime.Minute();
	tm_struct.tm_sec = fTime.Second();

	tm_struct.tm_year = fDate.Year() - 1900;
	tm_struct.tm_mon = fDate.Month() - 1;
	tm_struct.tm_mday = fDate.Day();

	// set less 0 as we wan't use it
	tm_struct.tm_isdst = -1;

	// return secs_since_jan1_1970 or -1 on error
	return int32(mktime(&tm_struct));
}


/*!
	Sets the current date and time converted from seconds since
	1.1.1970 - 00:00:00.
*/
void
BDateTime::SetTime_t(uint32 seconds)
{
	BTime time;
	time.AddSeconds(seconds % kSecondsPerDay);
	fTime.SetTime(time);
	
	BDate date(1970, 1, 1);
	date.AddDays(seconds / kSecondsPerDay);
	fDate.SetDate(date);
}


/*!
	Returns true if this datetime is different from \c dateTime, otherwise false.
*/
bool
BDateTime::operator!=(const BDateTime& dateTime) const
{
	return fTime != dateTime.fTime && fDate != dateTime.fDate;
}


/*!
	Returns true if this datetime is equal to \c dateTime, otherwise false.
*/
bool
BDateTime::operator==(const BDateTime& dateTime) const
{
	return fTime == dateTime.fTime && fDate == dateTime.fDate;
}


/*!
	Returns true if this datetime is earlier than \c dateTime, otherwise false.
*/
bool
BDateTime::operator<(const BDateTime& dateTime) const
{
	return fTime < dateTime.fTime && fDate < dateTime.fDate;
}


/*!
	Returns true if this datetime is earlier than or equal to \c dateTime,
	otherwise false.
*/
bool
BDateTime::operator<=(const BDateTime& dateTime) const
{
	return fTime <= dateTime.fTime && fDate <= dateTime.fDate;
}


/*!
	Returns true if this datetime is later than \c dateTime, otherwise false.
*/
bool
BDateTime::operator>(const BDateTime& dateTime) const
{
	return fTime > dateTime.fTime && fDate > dateTime.fDate;
}


/*!
	Returns true if this datetime is later than or equal to \c dateTime,
	otherwise false.
*/
bool
BDateTime::operator>=(const BDateTime& dateTime) const
{
	return fTime >= dateTime.fTime && fDate >= dateTime.fDate;
}


}	//namespace BPrivate
