# Licensed to the Software Freedom Conservancy (SFC) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The SFC licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import pytest

from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.actions.action_builder import ActionBuilder
from selenium.webdriver.support.ui import WebDriverWait


def test_should_be_able_to_get_pointer_and_keyboard_inputs(driver, pages):
    actions = ActionBuilder(driver)
    pointers = actions.pointer_inputs
    keyboards = actions.key_inputs

    assert pointers is not None
    assert keyboards is not None


@pytest.mark.xfail_firefox
def testSendingKeysToActiveElementWithModifier(driver, pages):
    pages.load("formPage.html")
    e = driver.find_element_by_id("working")
    e.click()

    actions = ActionBuilder(driver)
    key_action = actions.key_action
    key_action.key_down(Keys.SHIFT) \
        .send_keys("abc") \
        .key_up(Keys.SHIFT)

    actions.perform()

    assert "ABC" == e.get_attribute('value')


@pytest.mark.xfail_firefox
def test_can_create_pause_action_on_keyboard(driver, pages):
    # If we don't get an error and takes less than 3 seconds to run, we are good
    import datetime
    start = datetime.datetime.now()
    actions1 = ActionBuilder(driver)
    key_actions = actions1.key_action
    key_actions.pause(1)
    actions1.perform()
    finish = datetime.datetime.now()
    assert (finish - start).seconds <= 3

    # Add a filler step
    actions2 = ActionBuilder(driver)
    key_action = actions2.key_action
    key_action.pause()
    actions2.perform()


@pytest.mark.xfail_firefox
def test_can_create_pause_action_on_pointer(driver, pages):
    # If we don't get an error and takes less than 3 seconds to run, we are good
    import datetime
    start = datetime.datetime.now()
    actions1 = ActionBuilder(driver)
    key_actions = actions1.pointer_action
    key_actions.pause(1)
    actions1.perform()
    finish = datetime.datetime.now()
    assert (finish - start).seconds <= 3

    # Add a filler step
    actions2 = ActionBuilder(driver)
    key_action = actions2.pointer_action
    key_action.pause()
    actions2.perform()


@pytest.mark.xfail_firefox
def test_can_clear_actions(driver, pages):
    actions = ActionBuilder(driver)
    actions.clear_actions()


@pytest.mark.xfail_firefox
def test_move_and_click(driver, pages):
    pages.load("javascriptPage.html")
    toClick = driver.find_element_by_id("clickField")

    actions = ActionBuilder(driver)
    pointer = actions.pointer_action

    pointer.move_to(toClick) \
           .click()

    actions.perform()
    assert "Clicked" == toClick.get_attribute('value')


@pytest.mark.xfail_firefox
def testDragAndDrop(driver, pages):
    """Copied from org.openqa.selenium.interactions.TestBasicMouseInterface."""
    element_available_timeout = 15
    wait = WebDriverWait(driver, element_available_timeout)
    pages.load("droppableItems.html")
    wait.until(lambda dr: _isElementAvailable(driver, "draggable"))

    if not _isElementAvailable(driver, "draggable"):
        raise AssertionError("Could not find draggable element after 15 seconds.")

    toDrag = driver.find_element_by_id("draggable")
    dropInto = driver.find_element_by_id("droppable")
    actions = ActionBuilder(driver)
    pointer = actions.pointer_action
    pointer.click_and_hold(toDrag) \
           .move_to(dropInto)\
           .release()

    actions.perform()

    dropInto = driver.find_element_by_id("droppable")
    text = dropInto.find_element_by_tag_name("p").text
    assert "Dropped!" == text


@pytest.mark.xfail_firefox
def test_context_click(driver, pages):

    pages.load("javascriptPage.html")
    toContextClick = driver.find_element_by_id("doubleClickField")

    actions = ActionBuilder(driver)
    pointer = actions.pointer_action
    pointer.context_click(toContextClick)

    actions.perform()
    assert "ContextClicked" == toContextClick.get_attribute('value')


@pytest.mark.xfail_firefox
@pytest.mark.xfail_marionette(reason="Fails on Travis")
@pytest.mark.xfail_remote(reason="Fails on Travis")
def test_double_click(driver, pages):
    """Copied from org.openqa.selenium.interactions.TestBasicMouseInterface."""
    pages.load("javascriptPage.html")
    toDoubleClick = driver.find_element_by_id("doubleClickField")

    actions = ActionBuilder(driver)
    pointer = actions.pointer_action

    pointer.double_click(toDoubleClick)

    actions.perform()
    assert "DoubleClicked" == toDoubleClick.get_attribute('value')


@pytest.mark.xfail_firefox
def test_dragging_element_with_mouse_moves_it_to_another_list(driver, pages):
    _performDragAndDropWithMouse(driver, pages)
    dragInto = driver.find_element_by_id("sortable1")
    assert 6 == len(dragInto.find_elements_by_tag_name("li"))


@pytest.mark.xfail_firefox
def test_dragging_element_with_mouse_fires_events(driver, pages):
    _performDragAndDropWithMouse(driver, pages)
    dragReporter = driver.find_element_by_id("dragging_reports")
    assert "Nothing happened. DragOut DropIn RightItem 3" == dragReporter.text


def _performDragAndDropWithMouse(driver, pages):
    """Copied from org.openqa.selenium.interactions.TestBasicMouseInterface."""
    pages.load("draggableLists.html")
    dragReporter = driver.find_element_by_id("dragging_reports")
    toDrag = driver.find_element_by_id("rightitem-3")
    dragInto = driver.find_element_by_id("sortable1")

    actions = ActionBuilder(driver)
    pointer = actions.pointer_action
    pointer.click_and_hold(toDrag) \
           .move_to(driver.find_element_by_id("leftitem-4")) \
           .move_to(dragInto) \
           .release()

    assert "Nothing happened." == dragReporter.text

    actions.perform()
    assert "Nothing happened. DragOut" in dragReporter.text


def _isElementAvailable(driver, id):
    """Copied from org.openqa.selenium.interactions.TestBasicMouseInterface."""
    try:
        driver.find_element_by_id(id)
        return True
    except Exception:
        return False
