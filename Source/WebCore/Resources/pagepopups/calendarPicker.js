"use strict";
/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// FIXME:
//  - Touch event

/**
 * CSS class names.
 *
 * @enum {string}
 */
var ClassNames = {
    Available: "available",
    CancelButton: "cancel-button",
    ClearButton: "clear-button",
    Day: "day",
    DayLabel: "day-label",
    DayLabelContainer: "day-label-container",
    DaysArea: "days-area",
    DaysAreaContainer: "days-area-container",
    MonthSelector: "month-selector",
    MonthSelectorBox: "month-selector-box",
    MonthSelectorPopup: "month-selector-popup",
    MonthSelectorPopupContents: "month-selector-popup-contents",
    MonthSelectorPopupEntry: "month-selector-popup-entry",
    MonthSelectorWall: "month-selector-wall",
    NoFocusRing: "no-focus-ring",
    NotThisMonth: "not-this-month",
    Selected: "day-selected",
    SelectedMonthYear: "selected-month-year",
    TodayButton: "today-button",
    TodayClearArea: "today-clear-area",
    Unavailable: "unavailable",
    WeekContainer: "week-container",
    YearMonthArea: "year-month-area",
    YearMonthButton: "year-month-button",
    YearMonthButtonLeft: "year-month-button-left",
    YearMonthButtonRight: "year-month-button-right",
    YearMonthUpper: "year-month-upper"
};

/**
 * @type {Object}
 */
var global = {
    argumentsReceived: false,
    hadKeyEvent: false,
    params: null
};

// ----------------------------------------------------------------
// Utility functions

/**
 * @return {!string} lowercase locale name. e.g. "en-us"
 */
function getLocale() {
    return (global.params.locale || "en-us").toLowerCase();
}

/**
 * @return {!string} lowercase language code. e.g. "en"
 */
function getLanguage() {
    var locale = getLocale();
    var result = locale.match(/^([a-z]+)/);
    if (!result)
        return "en";
    return result[1];
}

/**
 * @param {!number} number
 * @return {!string}
 */
function localizeNumber(number) {
    return window.pagePopupController.localizeNumberString(number);
}

/*
 * @const
 * @type {number}
 */
var ImperialEraLimit = 2087;

/**
 * @param {!number} year
 * @param {!number} month
 * @return {!string}
 */
function formatJapaneseImperialEra(year, month) {
    // We don't show an imperial era if it is greater than 99 becase of space
    // limitation.
    if (year > ImperialEraLimit)
        return "";
    if (year > 1989)
        return "(平成" + localizeNumber(year - 1988) + "年)";
    if (year == 1989)
        return "(平成元年)";
    if (year >= 1927)
        return "(昭和" + localizeNumber(year - 1925) + "年)";
    if (year > 1912)
        return "(大正" + localizeNumber(year - 1911) + "年)";
    if (year == 1912 && month >= 7)
        return "(大正元年)";
    if (year > 1868)
        return "(明治" + localizeNumber(year - 1867) + "年)";
    if (year == 1868)
        return "(明治元年)";
    return "";
}

/**
 * @param {!number} year
 * @param {!number} month
 * @return {!string}
 */
function formatYearMonth(year, month) {
    var yearString = localizeNumber(year);
    var monthString = global.params.monthLabels[month];
    switch (getLanguage()) {
    case "eu":
    case "fil":
    case "lt":
    case "ml":
    case "mt":
    case "tl":
    case "ur":
        return yearString + " " + monthString;
    case "hu":
        return yearString + ". " + monthString;
    case "ja":
        return yearString + "年" + formatJapaneseImperialEra(year, month) + " " + monthString;
    case "zh":
        return yearString + "年" + monthString;
    case "ko":
        return yearString + "년 " + monthString;
    case "lv":
        return yearString + ". g. " + monthString;
    case "pt":
        return monthString + " de " + yearString;
    case "sr":
        return monthString + ". " + yearString;
    default:
        return monthString + " " + yearString;
    }
}

function createUTCDate(year, month, date) {
    var newDate = new Date(Date.UTC(year, month, date));
    if (year < 100)
        newDate.setUTCFullYear(year);
    return newDate;
};

/**
 * @param {string=} opt_current
 * @return {!Date}
 */
function parseDateString(opt_current) {
    if (opt_current) {
        var result = opt_current.match(/(\d+)-(\d+)-(\d+)/);
        if (result)
            return createUTCDate(Number(result[1]), Number(result[2]) - 1, Number(result[3]));
    }
    var now = new Date();
    // Create UTC date with same numbers as local date.
    return createUTCDate(now.getFullYear(), now.getMonth(), now.getDate());
}

/**
 * @param {!number} year
 * @param {!number} month
 * @param {!number} day
 * @return {!string}
 */
function serializeDate(year, month, day) {
    var yearString = String(year);
    if (yearString.length < 4)
        yearString = ("000" + yearString).substr(-4, 4);
    return yearString + "-" + ("0" + (month + 1)).substr(-2, 2) + "-" + ("0" + day).substr(-2, 2);
}

// ----------------------------------------------------------------
// Initialization

/**
 * @param {Event} event
 */
function handleMessage(event) {
    if (global.argumentsReceived)
        return;
    global.argumentsReceived = true;
    initialize(JSON.parse(event.data));
}

function handleArgumentsTimeout() {
    if (global.argumentsReceived)
        return;
    var args = {
        monthLabels : ["m1", "m2", "m3", "m4", "m5", "m6",
                       "m7", "m8", "m9", "m10", "m11", "m12"],
        dayLabels : ["d1", "d2", "d3", "d4", "d5", "d6", "d7"],
        todayLabel : "Today",
        clearLabel : "Clear",
        cancelLabel : "Cancel",
        currentValue : "",
        weekStartDay : 0,
        step : 1
    };
    initialize(args);
}

/**
 * @param {!Object} args
 * @return {?string} An error message, or null if the argument has no errors.
 */
function validateArguments(args) {
    if (!args.monthLabels)
        return "No monthLabels.";
    if (args.monthLabels.length != 12)
        return "monthLabels is not an array with 12 elements.";
    if (!args.dayLabels)
        return "No dayLabels.";
    if (args.dayLabels.length != 7)
        return "dayLabels is not an array with 7 elements.";
    if (!args.clearLabel)
        return "No clearLabel.";
    if (!args.todayLabel)
        return "No todayLabel.";
    if (args.weekStartDay) {
        if (args.weekStartDay < 0 || args.weekStartDay > 6)
            return "Invalid weekStartDay: " + args.weekStartDay;
    }
    return null;
}

/**
 * @param {!Object} args
 */
function initialize(args) {
    var main = $("main");
    main.classList.add(ClassNames.NoFocusRing);

    var errorString = validateArguments(args);
    if (errorString) {
        main.textContent = "Internal error: " + errorString;
        resizeWindow(main.offsetWidth, main.offsetHeight);
    } else {
        global.params = args;
        openCalendarPicker();
    }
}

function resetMain() {
    var main = $("main");
    main.innerHTML = "";
    main.className = "";
};

function openCalendarPicker() {
    resetMain();
    new CalendarPicker($("main"), global.params);
};

/**
 * @constructor
 * @param {!Element} element
 * @param {!Object} config
 */
function CalendarPicker(element, config) {
    Picker.call(this, element, config);
    // We assume this._config.min is a valid date.
    this.minimumDate = (typeof this._config.min !== "undefined") ? parseDateString(this._config.min) : CalendarPicker.MinimumPossibleDate;
    // We assume this._config.max is a valid date.
    this.maximumDate = (typeof this._config.max !== "undefined") ? parseDateString(this._config.max) : CalendarPicker.MaximumPossibleDate;
    this.step = (typeof this._config.step !== undefined) ? this._config.step * CalendarPicker.BaseStep : CalendarPicker.BaseStep;
    this.yearMonthController = new YearMonthController(this);
    this.daysTable = new DaysTable(this);
    this._layout();
    var initialDate = parseDateString(this._config.currentValue);
    if (initialDate < this.minimumDate)
        initialDate = this.minimumDate;
    else if (initialDate > this.maximumDate)
        initialDate = this.maximumDate;
    this.daysTable.selectDate(initialDate);
    this.fixWindowSize();
    document.body.addEventListener("keydown", this._handleBodyKeyDown.bind(this), false);
}
CalendarPicker.prototype = Object.create(Picker.prototype);

// Hard limits of type=date. See WebCore/platform/DateComponents.h.
CalendarPicker.MinimumPossibleDate = new Date(-62135596800000.0);
CalendarPicker.MaximumPossibleDate = new Date(8640000000000000.0);
// See WebCore/html/DateInputType.cpp.
CalendarPicker.BaseStep = 86400000;

CalendarPicker.prototype._layout = function() {
    this._element.style.direction = global.params.isRTL ? "rtl" : "ltr";
    this.yearMonthController.attachTo(this._element);
    this.daysTable.attachTo(this._element);
    this._layoutButtons();
};

CalendarPicker.prototype.handleToday = function() {
    var date = new Date();
    this.daysTable.selectDate(date);
    this.submitValue(serializeDate(date.getFullYear(), date.getMonth(), date.getDate()));
};

CalendarPicker.prototype.handleClear = function() {
    this.submitValue("");
};

CalendarPicker.prototype.fixWindowSize = function() {
    var yearMonthRightElement = this._element.getElementsByClassName(ClassNames.YearMonthButtonRight)[0];
    var daysAreaElement = this._element.getElementsByClassName(ClassNames.DaysArea)[0];
    var headers = daysAreaElement.getElementsByClassName(ClassNames.DayLabel);
    var maxCellWidth = 0;
    for (var i = 0; i < headers.length; ++i) {
        if (maxCellWidth < headers[i].offsetWidth)
            maxCellWidth = headers[i].offsetWidth;
    }
    var DaysAreaContainerBorder = 1;
    var yearMonthEnd;
    var daysAreaEnd;
    if (global.params.isRTL) {
        var startOffset = this._element.offsetLeft + this._element.offsetWidth;
        yearMonthEnd = startOffset - yearMonthRightElement.offsetLeft;
        daysAreaEnd = startOffset - (daysAreaElement.offsetLeft + daysAreaElement.offsetWidth) + maxCellWidth * 7 + DaysAreaContainerBorder;
    } else {
        yearMonthEnd = yearMonthRightElement.offsetLeft + yearMonthRightElement.offsetWidth;
        daysAreaEnd = daysAreaElement.offsetLeft + maxCellWidth * 7 + DaysAreaContainerBorder;
    }
    var maxEnd = Math.max(yearMonthEnd, daysAreaEnd);
    var MainPadding = 6; // FIXME: Fix name.
    var MainBorder = 1;
    var desiredBodyWidth = maxEnd + MainPadding + MainBorder;

    var elementHeight = this._element.offsetHeight;
    this._element.style.width = "auto";
    daysAreaElement.style.width = "100%";
    daysAreaElement.style.tableLayout = "fixed";
    this._element.getElementsByClassName(ClassNames.YearMonthUpper)[0].style.display = "-webkit-box";
    this._element.getElementsByClassName(ClassNames.MonthSelectorBox)[0].style.display = "block";
    resizeWindow(desiredBodyWidth, elementHeight);
};

CalendarPicker.prototype._layoutButtons = function() {
    var container = createElement("div", ClassNames.TodayClearArea);
    this.today = createElement("input", ClassNames.TodayButton);
    this.today.type = "button";
    this.today.value = this._config.todayLabel;
    this.today.addEventListener("click", this.handleToday.bind(this), false);
    container.appendChild(this.today);
    this.clear = null;
    if (!this._config.required) {
        this.clear = createElement("input", ClassNames.ClearButton);
        this.clear.type = "button";
        this.clear.value = this._config.clearLabel;
        this.clear.addEventListener("click", this.handleClear.bind(this), false);
        container.appendChild(this.clear);
    }
    this._element.appendChild(container);

    this.lastFocusableControl = this.clear || this.today;
};

// ----------------------------------------------------------------

/**
 * @constructor
 * @param {!CalendarPicker} picker
 */
function YearMonthController(picker) {
    this.picker = picker;
    /**
     * @type {!number}
     */
    this._currentYear = -1;
    /**
     * @type {!number}
     */
    this._currentMonth = -1;
}

/**
 * @param {!Element} element
 */
YearMonthController.prototype.attachTo = function(element) {
    var outerContainer = createElement("div", ClassNames.YearMonthArea);

    var innerContainer = createElement("div", ClassNames.YearMonthUpper);
    outerContainer.appendChild(innerContainer);

    this._attachLeftButtonsTo(innerContainer);

    var box = createElement("div", ClassNames.MonthSelectorBox);
    innerContainer.appendChild(box);
    // We can't use <select> popup in PagePopup.
    this._monthPopup = createElement("div", ClassNames.MonthSelectorPopup);
    this._monthPopup.addEventListener("click", this._handleYearMonthChange.bind(this), false);
    this._monthPopup.addEventListener("keydown", this._handleMonthPopupKey.bind(this), false);
    this._monthPopup.addEventListener("mousemove", this._handleMouseMove.bind(this), false);
    this._updateSelectionOnMouseMove = true;
    this._monthPopup.tabIndex = 0;
    this._monthPopupContents = createElement("div", ClassNames.MonthSelectorPopupContents);
    this._monthPopup.appendChild(this._monthPopupContents);
    box.appendChild(this._monthPopup);
    this._month = createElement("div", ClassNames.MonthSelector);
    this._month.addEventListener("click", this._showPopup.bind(this), false);
    box.appendChild(this._month);

    this._attachRightButtonsTo(innerContainer);
    element.appendChild(outerContainer);

    this._wall = createElement("div", ClassNames.MonthSelectorWall);
    this._wall.addEventListener("click", this._closePopup.bind(this), false);
    element.appendChild(this._wall);

    var maximumYear = this.picker.maximumDate.getUTCFullYear();
    var maxWidth = 0;
    for (var m = 0; m < 12; ++m) {
        this._month.textContent = formatYearMonth(maximumYear, m);
        maxWidth = Math.max(maxWidth, this._month.offsetWidth);
    }
    if (getLanguage() == "ja" && ImperialEraLimit < maximumYear) {
        for (var m = 0; m < 12; ++m) {
            this._month.textContent = formatYearMonth(ImperialEraLimit, m);
            maxWidth = Math.max(maxWidth, this._month.offsetWidth);
        }
    }
    this._month.style.minWidth = maxWidth + 'px';

    global.firstFocusableControl = this._left2; // FIXME: Should it be this.month?
};

YearMonthController.addTenYearsButtons = false;

/**
 * @param {!Element} parent
 */
YearMonthController.prototype._attachLeftButtonsTo = function(parent) {
    var container = createElement("div", ClassNames.YearMonthButtonLeft);
    parent.appendChild(container);

    if (YearMonthController.addTenYearsButtons) {
        this._left3 = createElement("input", ClassNames.YearMonthButton);
        this._left3.type = "button";
        this._left3.value = "<<<";
        this._left3.addEventListener("click", this._handleButtonClick.bind(this), false);
        container.appendChild(this._left3);
    }

    this._left2 = createElement("input", ClassNames.YearMonthButton);
    this._left2.type = "button";
    this._left2.value = "<<";
    this._left2.addEventListener("click", this._handleButtonClick.bind(this), false);
    container.appendChild(this._left2);

    this._left1 = createElement("input", ClassNames.YearMonthButton);
    this._left1.type = "button";
    this._left1.value = "<";
    this._left1.addEventListener("click", this._handleButtonClick.bind(this), false);
    container.appendChild(this._left1);
};

/**
 * @param {!Element} parent
 */
YearMonthController.prototype._attachRightButtonsTo = function(parent) {
    var container = createElement("div", ClassNames.YearMonthButtonRight);
    parent.appendChild(container);
    this._right1 = createElement("input", ClassNames.YearMonthButton);
    this._right1.type = "button";
    this._right1.value = ">";
    this._right1.addEventListener("click", this._handleButtonClick.bind(this), false);
    container.appendChild(this._right1);

    this._right2 = createElement("input", ClassNames.YearMonthButton);
    this._right2.type = "button";
    this._right2.value = ">>";
    this._right2.addEventListener("click", this._handleButtonClick.bind(this), false);
    container.appendChild(this._right2);

    if (YearMonthController.addTenYearsButtons) {
        this._right3 = createElement("input", ClassNames.YearMonthButton);
        this._right3.type = "button";
        this._right3.value = ">>>";
        this._right3.addEventListener("click", this._handleButtonClick.bind(this), false);
        container.appendChild(this._right3);
    }
};

/**
 * @return {!number}
 */
YearMonthController.prototype.year = function() {
    return this._currentYear;
};

/**
 * @return {!number}
 */
YearMonthController.prototype.month = function() {
    return this._currentMonth;
};

/**
 * @param {!number} year
 * @param {!number} month
 */
YearMonthController.prototype.setYearMonth = function(year, month) {
    this._currentYear = year;
    this._currentMonth = month;
    this._redraw();
};

YearMonthController.prototype._redraw = function() {
    var min = this.picker.minimumDate.getUTCFullYear() * 12 + this.picker.minimumDate.getUTCMonth();
    var max = this.picker.maximumDate.getUTCFullYear() * 12 + this.picker.maximumDate.getUTCMonth();
    var current = this._currentYear * 12 + this._currentMonth;
    if (this._left3)
        this._left3.disabled = current - 13 < min;
    this._left2.disabled = current - 2 < min;
    this._left1.disabled = current - 1 < min;
    this._right1.disabled = current + 1 > max;
    this._right2.disabled = current + 2 > max;
    if (this._right3)
        this._right3.disabled = current + 13 > max;
    this._month.innerText = formatYearMonth(this._currentYear, this._currentMonth);
    while (this._monthPopupContents.hasChildNodes())
        this._monthPopupContents.removeChild(this._monthPopupContents.firstChild);

    for (var m = current - 6; m <= current + 6; m++) {
        if (m < min || m > max)
            continue;
        var option = createElement("div", ClassNames.MonthSelectorPopupEntry, formatYearMonth(Math.floor(m / 12), m % 12));
        option.dataset.value = String(Math.floor(m / 12)) + "-" + String(m % 12);
        this._monthPopupContents.appendChild(option);
        if (m == current)
            option.classList.add(ClassNames.SelectedMonthYear);
    }
};

YearMonthController.prototype._showPopup = function() {
    this._monthPopup.style.display = "block";
    this._monthPopup.style.zIndex = "1000"; // Larger than the days area.
    this._monthPopup.style.left = this._month.offsetLeft + (this._month.offsetWidth - this._monthPopup.offsetWidth) / 2 + "px";
    this._monthPopup.style.top = this._month.offsetTop + this._month.offsetHeight + "px";

    this._wall.style.display = "block";
    this._wall.style.zIndex = "999"; // This should be smaller than the z-index of monthPopup.

    var popupHeight = this._monthPopup.clientHeight;
    var fullHeight = this._monthPopupContents.clientHeight;
    if (fullHeight > popupHeight) {
        var selected = this._getSelection();
        if (selected) {
           var bottom = selected.offsetTop + selected.clientHeight;
           if (bottom > popupHeight)
               this._monthPopup.scrollTop = bottom - popupHeight;
        }
        this._monthPopup.style.webkitPaddingEnd = getScrollbarWidth() + 'px';
    }
    this._monthPopup.focus();
};

YearMonthController.prototype._closePopup = function() {
    this._monthPopup.style.display = "none";
    this._wall.style.display = "none";
    var container = document.querySelector("." + ClassNames.DaysAreaContainer);
    container.focus();
};

/**
 * @return {Element} Selected element in the month-year popup.
 */
YearMonthController.prototype._getSelection = function()
{
    return document.querySelector("." + ClassNames.SelectedMonthYear);
}

/**
 * @param {Event} event
 */
YearMonthController.prototype._handleMouseMove = function(event)
{
    if (!this._updateSelectionOnMouseMove) {
        // Selection update turned off while navigating with keyboard to prevent a mouse
        // move trigged during a scroll from resetting the selection. Automatically
        // rearm control to enable mouse-based selection.
        this._updateSelectionOnMouseMove = true;
    } else {
        var target = event.target;
        var selection = this._getSelection();
        if (target && target != selection && target.classList.contains(ClassNames.MonthSelectorPopupEntry)) {
            if (selection)
                selection.classList.remove(ClassNames.SelectedMonthYear);
            target.classList.add(ClassNames.SelectedMonthYear);
        }
    }
    event.stopPropagation();
    event.preventDefault();
}

/**
 * @param {Event} event
 */
YearMonthController.prototype._handleMonthPopupKey = function(event)
{
    var key = event.keyIdentifier;
    if (key == "Down") {
        var selected = this._getSelection();
        if (selected) {
            var next = selected.nextSibling;
            if (next) {
                selected.classList.remove(ClassNames.SelectedMonthYear);
                next.classList.add(ClassNames.SelectedMonthYear);
                var bottom = next.offsetTop + next.clientHeight;
                if (bottom > this._monthPopup.scrollTop + this._monthPopup.clientHeight) {
                    this._updateSelectionOnMouseMove = false;
                    this._monthPopup.scrollTop = bottom - this._monthPopup.clientHeight;
                }
            }
        }
        event.stopPropagation();
        event.preventDefault();
    } else if (key == "Up") {
        var selected = this._getSelection();
        if (selected) {
            var previous = selected.previousSibling;
            if (previous) {
                selected.classList.remove(ClassNames.SelectedMonthYear);
                previous.classList.add(ClassNames.SelectedMonthYear);
                if (previous.offsetTop < this._monthPopup.scrollTop) {
                    this._updateSelectionOnMouseMove = false;
                    this._monthPopup.scrollTop = previous.offsetTop;
                }
            }
        }
        event.stopPropagation();
        event.preventDefault();
    } else if (key == "U+001B") {
        this._closePopup();
        event.stopPropagation();
        event.preventDefault();
    } else if (key == "Enter") {
        this._handleYearMonthChange();
        event.stopPropagation();
        event.preventDefault();
    }
}

YearMonthController.prototype._handleYearMonthChange = function() {
    this._closePopup();
    var selection = this._getSelection();
    if (!selection)
        return;
    var value = selection.dataset.value;
    var result  = value.match(/(\d+)-(\d+)/);
    if (!result)
        return;
    var newYear = Number(result[1]);
    var newMonth = Number(result[2]);
    this.picker.daysTable.navigateToMonthAndKeepSelectionPosition(newYear, newMonth);
};

/*
 * @const
 * @type {number}
 */
YearMonthController.PreviousTenYears = -120;
/*
 * @const
 * @type {number}
 */
YearMonthController.PreviousYear = -12;
/*
 * @const
 * @type {number}
 */
YearMonthController.PreviousMonth = -1;
/*
 * @const
 * @type {number}
 */
YearMonthController.NextMonth = 1;
/*
 * @const
 * @type {number}
 */
YearMonthController.NextYear = 12;
/*
 * @const
 * @type {number}
 */
YearMonthController.NextTenYears = 120;

/**
 * @param {Event} event
 */
YearMonthController.prototype._handleButtonClick = function(event) {
    if (event.target == this._left3)
        this.moveRelatively(YearMonthController.PreviousTenYears);
    else if (event.target == this._left2)
        this.moveRelatively(YearMonthController.PreviousYear);
    else if (event.target == this._left1)
        this.moveRelatively(YearMonthController.PreviousMonth);
    else if (event.target == this._right1)
        this.moveRelatively(YearMonthController.NextMonth)
    else if (event.target == this._right2)
        this.moveRelatively(YearMonthController.NextYear);
    else if (event.target == this._right3)
        this.moveRelatively(YearMonthController.NextTenYears);
    else
        return;
};

/**
 * @param {!number} amount
 */
YearMonthController.prototype.moveRelatively = function(amount) {
    var min = this.picker.minimumDate.getUTCFullYear() * 12 + this.picker.minimumDate.getUTCMonth();
    var max = this.picker.maximumDate.getUTCFullYear() * 12 + this.picker.maximumDate.getUTCMonth();
    var current = this._currentYear * 12 + this._currentMonth;
    var updated = current;
    if (amount < 0)
        updated = current + amount >= min ? current + amount : min;
    else
        updated = current + amount <= max ? current + amount : max;
    if (updated == current)
        return;
    this.picker.daysTable.navigateToMonthAndKeepSelectionPosition(Math.floor(updated / 12), updated % 12);
};

// ----------------------------------------------------------------

/**
 * @constructor
 * @param {!CalendarPicker} picker
 */
function DaysTable(picker) {
    this.picker = picker;
    /**
     * @type {!number}
     */
    this._x = -1;
    /**
     * @type {!number}
     */
    this._y = -1;
    /**
     * @type {!number}
     */
    this._currentYear = -1;
    /**
     * @type {!number}
     */
    this._currentMonth = -1;
}

/**
 * @return {!boolean}
 */
DaysTable.prototype._hasSelection = function() {
    return this._x >= 0 && this._y >= 0;
}

/**
 * The number of week lines in the screen.
 * @const
 * @type {number}
 */
DaysTable._Weeks = 6;

/**
 * @param {!Element} element
 */
DaysTable.prototype.attachTo = function(element) {
    this._daysContainer = createElement("table", ClassNames.DaysArea);
    this._daysContainer.addEventListener("click", this._handleDayClick.bind(this), false);
    this._daysContainer.addEventListener("mouseover", this._handleMouseOver.bind(this), false);
    this._daysContainer.addEventListener("mouseout", this._handleMouseOut.bind(this), false);
    this._daysContainer.addEventListener("webkitTransitionEnd", this._moveInDays.bind(this), false);
    var container = createElement("tr", ClassNames.DayLabelContainer);
    var weekStartDay = global.params.weekStartDay || 0;
    for (var i = 0; i < 7; i++)
        container.appendChild(createElement("th", ClassNames.DayLabel, global.params.dayLabels[(weekStartDay + i) % 7]));
    this._daysContainer.appendChild(container);
    this._days = [];
    for (var w = 0; w < DaysTable._Weeks; w++) {
        container = createElement("tr", ClassNames.WeekContainer);
        var week = [];
        for (var d = 0; d < 7; d++) {
            var day = createElement("td", ClassNames.Day, " ");
            day.setAttribute("data-position-x", String(d));
            day.setAttribute("data-position-y", String(w));
            week.push(day);
            container.appendChild(day);
        }
        this._days.push(week);
        this._daysContainer.appendChild(container);
    }
    container = createElement("div", ClassNames.DaysAreaContainer);
    container.appendChild(this._daysContainer);
    container.tabIndex = 0;
    container.addEventListener("keydown", this._handleKey.bind(this), false);
    element.appendChild(container);

    container.focus();
};

/**
 * @param {!number} time date in millisecond.
 * @return {!boolean}
 */
CalendarPicker.prototype.stepMismatch = function(time) {
    return (time - this.minimumDate.getTime()) % this.step != 0;
}

/**
 * @param {!number} time date in millisecond.
 * @return {!boolean}
 */
CalendarPicker.prototype.outOfRange = function(time) {
    return time < this.minimumDate.getTime() || time > this.maximumDate.getTime();
}

/**
 * @param {!number} time date in millisecond.
 * @return {!boolean}
 */
CalendarPicker.prototype.isValidDate = function(time) {
    return !this.outOfRange(time) && !this.stepMismatch(time);
}

/**
 * @param {!number} year
 * @param {!number} month
 */
DaysTable.prototype._renderMonth = function(year, month) {
    this._currentYear = year;
    this._currentMonth = month;
    var dayIterator = createUTCDate(year, month, 1);
    var monthStartDay = dayIterator.getUTCDay();
    var weekStartDay = global.params.weekStartDay || 0;
    var startOffset = weekStartDay - monthStartDay;
    if (startOffset >= 0)
        startOffset -= 7;
    dayIterator.setUTCDate(startOffset + 1);
    for (var w = 0; w < DaysTable._Weeks; w++) {
        for (var d = 0; d < 7; d++) {
            var iterYear = dayIterator.getUTCFullYear();
            var iterMonth = dayIterator.getUTCMonth();
            var time = dayIterator.getTime();
            var element = this._days[w][d];
            element.innerText = localizeNumber(dayIterator.getUTCDate());
            element.className = ClassNames.Day;
            element.dataset.submitValue = serializeDate(iterYear, iterMonth, dayIterator.getUTCDate());
            if (this.picker.outOfRange(time))
                element.classList.add(ClassNames.Unavailable);
            else if (this.picker.stepMismatch(time))
                element.classList.add(ClassNames.Unavailable);
            else if ((iterYear == year && dayIterator.getUTCMonth() < month) || (month == 0 && iterMonth == 11)) {
                element.classList.add(ClassNames.Available);
                element.classList.add(ClassNames.NotThisMonth);
            } else if ((iterYear == year && dayIterator.getUTCMonth() > month) || (month == 11 && iterMonth == 0)) {
                element.classList.add(ClassNames.Available);
                element.classList.add(ClassNames.NotThisMonth);
            } else if (isNaN(time)) {
                element.innerText = "-";
                element.classList.add(ClassNames.Unavailable);
            } else
                element.classList.add(ClassNames.Available);
            dayIterator.setUTCDate(dayIterator.getUTCDate() + 1);
        }
    }

    this.picker.today.disabled = !this.picker.isValidDate(parseDateString().getTime());
};

/**
 * @param {!number} year
 * @param {!number} month
 */
DaysTable.prototype._navigateToMonth = function(year, month) {
    this.picker.yearMonthController.setYearMonth(year, month);
    this._renderMonth(year, month);
};

/**
 * @param {!number} year
 * @param {!number} month
 */
DaysTable.prototype._navigateToMonthWithAnimation = function(year, month) {
    if (this._currentYear >= 0 && this._currentMonth >= 0) {
        if (year == this._currentYear && month == this._currentMonth)
            return;
        var decreasing = false;
        if (year < this._currentYear)
            decreasing = true;
        else if (year > this._currentYear)
            decreasing = false;
        else
            decreasing = month < this._currentMonth;
        var daysStyle = this._daysContainer.style;
        daysStyle.position = "relative";
        daysStyle.webkitTransition = "left 0.1s ease";
        daysStyle.left = (decreasing ? "" : "-") + this._daysContainer.offsetWidth + "px";
    }
    this._navigateToMonth(year, month);
};

DaysTable.prototype._moveInDays = function() {
    var daysStyle = this._daysContainer.style;
    if (daysStyle.left == "0px")
        return;
    daysStyle.webkitTransition = "";
    daysStyle.left = (daysStyle.left.charAt(0) == "-" ? "" : "-") + this._daysContainer.offsetWidth + "px";
    this._daysContainer.offsetLeft; // Force to layout.
    daysStyle.webkitTransition = "left 0.1s ease";
    daysStyle.left = "0px";
};

/**
 * @param {!number} year
 * @param {!number} month
 */
DaysTable.prototype.navigateToMonthAndKeepSelectionPosition = function(year, month) {
    if (year == this._currentYear && month == this._currentMonth)
        return;
    this._navigateToMonthWithAnimation(year, month);
    if (this._hasSelection())
        this._days[this._y][this._x].classList.add(ClassNames.Selected);
};

/**
 * @param {!Date} date
 */
DaysTable.prototype.selectDate = function(date) {
    this._navigateToMonthWithAnimation(date.getUTCFullYear(), date.getUTCMonth());
    var dateString = serializeDate(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate());
    for (var w = 0; w < DaysTable._Weeks; w++) {
        for (var d = 0; d < 7; d++) {
            if (this._days[w][d].dataset.submitValue == dateString) {
                this._days[w][d].classList.add(ClassNames.Selected);
                this._x = d;
                this._y = w;
                break;
            }
        }
    }
};

/**
 * @return {!boolean}
 */
DaysTable.prototype._maybeSetPreviousMonth = function() {
    var year = global.yearMonthController.year();
    var month = global.yearMonthController.month();
    var thisMonthStartTime = createUTCDate(year, month, 1).getTime();
    if (this.minimumDate.getTime() >= thisMonthStartTime)
        return false;
    if (month == 0) {
        year--;
        month = 11;
    } else
        month--;
    this._navigateToMonthWithAnimation(year, month);
    return true;
};

/**
 * @return {!boolean}
 */
DaysTable.prototype._maybeSetNextMonth = function() {
    var year = global.yearMonthController.year();
    var month = global.yearMonthController.month();
    if (month == 11) {
        year++;
        month = 0;
    } else
        month++;
    var nextMonthStartTime = createUTCDate(year, month, 1).getTime();
    if (this.picker.maximumDate.getTime() < nextMonthStartTime)
        return false;
    this._navigateToMonthWithAnimation(year, month);
    return true;
};

/**
 * @param {Event} event
 */
DaysTable.prototype._handleDayClick = function(event) {
    if (event.target.classList.contains(ClassNames.Available))
        this.picker.submitValue(event.target.dataset.submitValue);
};

/**
 * @param {Event} event
 */
DaysTable.prototype._handleMouseOver = function(event) {
    var node = event.target;
    if (this._hasSelection())
        this._days[this._y][this._x].classList.remove(ClassNames.Selected);
    if (!node.classList.contains(ClassNames.Day)) {
        this._x = -1;
        this._y = -1;
        return;
    }
    node.classList.add(ClassNames.Selected);
    this._x = Number(node.dataset.positionX);
    this._y = Number(node.dataset.positionY);
};

/**
 * @param {Event} event
 */
DaysTable.prototype._handleMouseOut = function(event) {
    if (this._hasSelection())
        this._days[this._y][this._x].classList.remove(ClassNames.Selected);
    this._x = -1;
    this._y = -1;
};

/**
 * @param {Event} event
 */
DaysTable.prototype._handleKey = function(event) {
    maybeUpdateFocusStyle();
    var x = this._x;
    var y = this._y;
    var key = event.keyIdentifier;
    if (!this._hasSelection() && (key == "Left" || key == "Up" || key == "Right" || key == "Down")) {
        // Put the selection on a center cell.
        this.updateSelection(event, 3, Math.floor(DaysTable._Weeks / 2 - 1));
        return;
    }

    if (key == (global.params.isRTL ? "Right" : "Left")) {
        if (x == 0) {
            if (y == 0) {
                if (!this._maybeSetPreviousMonth())
                    return;
                y = DaysTable._Weeks - 1;
            } else
                y--;
            x = 6;
        } else
            x--;
        this.updateSelection(event, x, y);

    } else if (key == "Up") {
        if (y == 0) {
            if (!this._maybeSetPreviousMonth())
                return;
            y = DaysTable._Weeks - 1;
        } else
            y--;
        this.updateSelection(event, x, y);

    } else if (key == (global.params.isRTL ? "Left" : "Right")) {
        if (x == 6) {
            if (y == DaysTable._Weeks - 1) {
                if (!this._maybeSetNextMonth())
                    return;
                y = 0;
            } else
                y++;
            x = 0;
        } else
            x++;
        this.updateSelection(event, x, y);

    } else if (key == "Down") {
        if (y == DaysTable._Weeks - 1) {
            if (!this._maybeSetNextMonth())
                return;
            y = 0;
        } else
            y++;
        this.updateSelection(event, x, y);

    } else if (key == "PageUp") {
        if (!this._maybeSetPreviousMonth())
            return;
        this.updateSelection(event, x, y);

    } else if (key == "PageDown") {
        if (!this._maybeSetNextMonth())
            return;
        this.updateSelection(event, x, y);

    } else if (this._hasSelection() && key == "Enter") {
        var dayNode = this._days[y][x];
        if (dayNode.classList.contains(ClassNames.Available)) {
            this.picker.submitValue(dayNode.dataset.submitValue);
            event.stopPropagation();
        }

    } else if (key == "U+0054") { // 't'
        this._days[this._y][this._x].classList.remove(ClassNames.Selected);
        this.selectDate(new Date());
        event.stopPropagation();
        event.preventDefault();
    }
};

/**
 * @param {Event} event
 * @param {!number} x
 * @param {!number} y
 */
DaysTable.prototype.updateSelection = function(event, x, y) {
    if (this._hasSelection())
        this._days[this._y][this._x].classList.remove(ClassNames.Selected);
    if (x >= 0 && y >= 0) {
        this._days[y][x].classList.add(ClassNames.Selected);
        this._x = x;
        this._y = y;
    }
    event.stopPropagation();
    event.preventDefault();
};

/**
 * @param {!Event} event
 */
CalendarPicker.prototype._handleBodyKeyDown = function(event) {
    maybeUpdateFocusStyle();
    var key = event.keyIdentifier;
    if (key == "U+0009") {
        if (!event.shiftKey && document.activeElement == global.lastFocusableControl) {
            event.stopPropagation();
            event.preventDefault();
            this.firstFocusableControl.focus();
        } else if (event.shiftKey && document.activeElement == global.firstFocusableControl) {
            event.stopPropagation();
            event.preventDefault();
            this.lastFocusableControl.focus();
        }
    } else if (key == "U+004D") { // 'm'
        this.yearMonthController.moveRelatively(event.shiftKey ? YearMonthController.PreviousMonth : YearMonthController.NextMonth);
    } else if (key == "U+0059") { // 'y'
        this.yearMonthController.moveRelatively(event.shiftKey ? YearMonthController.PreviousYear : YearMonthController.NextYear);
    } else if (key == "U+0044") { // 'd'
        this.yearMonthController.moveRelatively(event.shiftKey ? YearMonthController.PreviousTenYears : YearMonthController.NextTenYears);
    } else if (key == "U+001B") // ESC
        this.handleCancel();
}

function maybeUpdateFocusStyle() {
    if (global.hadKeyEvent)
        return;
    global.hadKeyEvent = true;
    $("main").classList.remove(ClassNames.NoFocusRing);
}

if (window.dialogArguments) {
    initialize(dialogArguments);
} else {
    window.addEventListener("message", handleMessage, false);
    window.setTimeout(handleArgumentsTimeout, 1000);
}
