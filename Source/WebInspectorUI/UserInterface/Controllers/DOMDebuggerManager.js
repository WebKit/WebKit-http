/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WI.DOMDebuggerManager = class DOMDebuggerManager extends WI.Object
{
    constructor()
    {
        super();

        this._domBreakpointURLMap = new Multimap;
        this._domBreakpointFrameIdentifierMap = new Map;

        this._listenerBreakpoints = [];
        this._urlBreakpoints = [];

        this._allAnimationFramesBreakpointEnabledSetting = new WI.Setting("break-on-all-animation-frames", false);
        this._allAnimationFramesBreakpoint = new WI.EventBreakpoint(WI.EventBreakpoint.Type.AnimationFrame, {
            disabled: !this._allAnimationFramesBreakpointEnabledSetting.value,
        });

        this._allIntervalsBreakpointEnabledSetting = new WI.Setting("break-on-all-intervals", false);
        this._allIntervalsBreakpoint = new WI.EventBreakpoint(WI.EventBreakpoint.Type.Interval, {
            disabled: !this._allIntervalsBreakpointEnabledSetting.value,
        });

        this._allListenersBreakpointEnabledSetting = new WI.Setting("break-on-all-listeners", false);
        this._allListenersBreakpoint = new WI.EventBreakpoint(WI.EventBreakpoint.Type.Listener, {
            disabled: !this._allListenersBreakpointEnabledSetting.value,
        });

        this._allTimeoutsBreakpointEnabledSetting = new WI.Setting("break-on-all-timeouts", false);
        this._allTimeoutsBreakpoint = new WI.EventBreakpoint(WI.EventBreakpoint.Type.Timeout, {
            disabled: !this._allTimeoutsBreakpointEnabledSetting.value,
        });

        this._allRequestsBreakpointEnabledSetting = new WI.Setting("break-on-all-requests", false);
        this._allRequestsBreakpoint = new WI.URLBreakpoint(WI.URLBreakpoint.Type.Text, "", {
            disabled: !this._allRequestsBreakpointEnabledSetting.value,
        });

        WI.DOMBreakpoint.addEventListener(WI.DOMBreakpoint.Event.DisabledStateChanged, this._handleDOMBreakpointDisabledStateChanged, this);
        WI.EventBreakpoint.addEventListener(WI.EventBreakpoint.Event.DisabledStateChanged, this._handleEventBreakpointDisabledStateChanged, this);
        WI.URLBreakpoint.addEventListener(WI.URLBreakpoint.Event.DisabledStateChanged, this._handleURLBreakpointDisabledStateChanged, this);

        WI.domManager.addEventListener(WI.DOMManager.Event.NodeRemoved, this._nodeRemoved, this);
        WI.domManager.addEventListener(WI.DOMManager.Event.NodeInserted, this._nodeInserted, this);

        WI.networkManager.addEventListener(WI.NetworkManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);

        WI.Frame.addEventListener(WI.Frame.Event.ChildFrameWasRemoved, this._childFrameWasRemoved, this);
        WI.Frame.addEventListener(WI.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

        let loadBreakpoints = (constructor, objectStore, oldSettings, callback) => {
            WI.Target.registerInitializationPromise((async () => {
                for (let key of oldSettings) {
                    let existingSerializedBreakpoints = WI.Setting.migrateValue(key);
                    if (existingSerializedBreakpoints) {
                        for (let existingSerializedBreakpoint of existingSerializedBreakpoints)
                            await objectStore.putObject(constructor.deserialize(existingSerializedBreakpoint));
                    }
                }

                let serializedBreakpoints = await objectStore.getAll();

                this._restoringBreakpoints = true;
                for (let serializedBreakpoint of serializedBreakpoints) {
                    let breakpoint = constructor.deserialize(serializedBreakpoint);

                    const key = null;
                    objectStore.associateObject(breakpoint, key, serializedBreakpoint);

                    callback(breakpoint);
                }
                this._restoringBreakpoints = false;
            })());
        };

        if (DOMDebuggerManager.supportsDOMBreakpoints()) {
            loadBreakpoints(WI.DOMBreakpoint, WI.objectStores.domBreakpoints, ["dom-breakpoints"], (breakpoint) => {
                this.addDOMBreakpoint(breakpoint);
            });
        }

        if (DOMDebuggerManager.supportsEventBreakpoints() || DOMDebuggerManager.supportsEventListenerBreakpoints()) {
            loadBreakpoints(WI.EventBreakpoint, WI.objectStores.eventBreakpoints, ["event-breakpoints"], (breakpoint) => {
                // Migrate `requestAnimationFrame`, `setTimeout`, and `setInterval` global breakpoints.
                switch (breakpoint.type) {
                case WI.EventBreakpoint.Type.AnimationFrame:
                    this._allAnimationFramesBreakpoint.disabled = breakpoint.disabled;
                    if (!WI.settings.showAllAnimationFramesBreakpoint.value) {
                        WI.settings.showAllAnimationFramesBreakpoint.value = true;
                        this.addEventBreakpoint(this._allAnimationFramesBreakpoint);
                    }
                    WI.objectStores.eventBreakpoints.deleteObject(breakpoint);
                    return;

                case WI.EventBreakpoint.Type.Timer:
                    switch (breakpoint.eventName) {
                    case "setTimeout":
                        this._allTimeoutsBreakpoint.disabled = breakpoint.disabled;
                        if (!WI.settings.showAllTimeoutsBreakpoint.value) {
                            WI.settings.showAllTimeoutsBreakpoint.value = true;
                            this.addEventBreakpoint(this._allTimeoutsBreakpoint);
                        }
                        break;

                    case "setInterval":
                        this._allIntervalsBreakpoint.disabled = breakpoint.disabled;
                        if (!WI.settings.showAllIntervalsBreakpoint.value) {
                            WI.settings.showAllIntervalsBreakpoint.value = true;
                            this.addEventBreakpoint(this._allIntervalsBreakpoint);
                        }
                        break;
                    }

                    WI.objectStores.eventBreakpoints.deleteObject(breakpoint);
                    return;
                }

                this.addEventBreakpoint(breakpoint);
            });
        }

        if (DOMDebuggerManager.supportsURLBreakpoints() || DOMDebuggerManager.supportsXHRBreakpoints()) {
            loadBreakpoints(WI.URLBreakpoint, WI.objectStores.urlBreakpoints, ["xhr-breakpoints", "url-breakpoints"], (breakpoint) => {
                this.addURLBreakpoint(breakpoint);
            });
        }
    }

    // Target

    initializeTarget(target)
    {
        if (target.hasDomain("DOMDebugger")) {
            this._restoringBreakpoints = true;

            if (target === WI.assumingMainTarget() && target.mainResource)
                this._speculativelyResolveDOMBreakpointsForURL(target.mainResource.url);

            if (!this._allAnimationFramesBreakpoint.disabled)
                this._updateEventBreakpoint(this._allAnimationFramesBreakpoint, target);

            if (!this._allIntervalsBreakpoint.disabled)
                this._updateEventBreakpoint(this._allIntervalsBreakpoint, target);

            if (!this._allListenersBreakpoint.disabled)
                this._updateEventBreakpoint(this._allListenersBreakpoint, target);

            if (!this._allTimeoutsBreakpoint.disabled)
                this._updateEventBreakpoint(this._allTimeoutsBreakpoint, target);

            if (!this._allRequestsBreakpoint.disabled)
                this._updateURLBreakpoint(this._allRequestsBreakpoint, target);

            for (let breakpoint of this._listenerBreakpoints) {
                if (!breakpoint.disabled)
                    this._updateEventBreakpoint(breakpoint, target);
            }

            for (let breakpoint of this._urlBreakpoints) {
                if (!breakpoint.disabled)
                    this._updateURLBreakpoint(breakpoint, target);
            }

            this._restoringBreakpoints = false;
        }
    }

    // Static

    static supportsDOMBreakpoints()
    {
        return InspectorBackend.hasCommand("DOMDebugger.setDOMBreakpoint")
            && InspectorBackend.hasCommand("DOMDebugger.removeDOMBreakpoint");
    }

    static supportsEventBreakpoints()
    {
        // COMPATIBILITY (iOS 13): DOMDebugger.setEventBreakpoint and DOMDebugger.removeEventBreakpoint did not exist yet.
        return InspectorBackend.hasCommand("DOMDebugger.setEventBreakpoint")
            && InspectorBackend.hasCommand("DOMDebugger.removeEventBreakpoint");
    }

    static supportsEventListenerBreakpoints()
    {
        // COMPATIBILITY (iOS 12.2): Replaced by DOMDebugger.setEventBreakpoint and DOMDebugger.removeEventBreakpoint.
        return InspectorBackend.hasCommand("DOMDebugger.setEventListenerBreakpoint")
            && InspectorBackend.hasCommand("DOMDebugger.removeEventListenerBreakpoint");
    }

    static supportsURLBreakpoints()
    {
        // COMPATIBILITY (iOS 13): DOMDebugger.setURLBreakpoint and DOMDebugger.removeURLBreakpoint did not exist yet.
        return InspectorBackend.hasCommand("DOMDebugger.setURLBreakpoint")
            && InspectorBackend.hasCommand("DOMDebugger.removeURLBreakpoint");
    }

    static supportsXHRBreakpoints()
    {
        // COMPATIBILITY (iOS 13): Replaced by DOMDebugger.setURLBreakpoint and DOMDebugger.removeURLBreakpoint.
        return InspectorBackend.hasCommand("DOMDebugger.setXHRBreakpoint")
            && InspectorBackend.hasCommand("DOMDebugger.removeXHRBreakpoint");
    }

    static supportsAllListenersBreakpoint()
    {
        // COMPATIBILITY (iOS 13): DOMDebugger.EventBreakpointType.Interval and DOMDebugger.EventBreakpointType.Timeout did not exist yet.
        return DOMDebuggerManager.supportsEventBreakpoints()
            && InspectorBackend.Enum.DOMDebugger.EventBreakpointType.Interval
            && InspectorBackend.Enum.DOMDebugger.EventBreakpointType.Timeout;
    }

    // Public

    get supported()
    {
        return InspectorBackend.hasDomain("DOMDebugger");
    }

    get allAnimationFramesBreakpoint() { return this._allAnimationFramesBreakpoint; }
    get allIntervalsBreakpoint() { return this._allIntervalsBreakpoint; }
    get allListenersBreakpoint() { return this._allListenersBreakpoint; }
    get allTimeoutsBreakpoint() { return this._allTimeoutsBreakpoint; }
    get allRequestsBreakpoint() { return this._allRequestsBreakpoint; }

    get domBreakpoints()
    {
        let mainFrame = WI.networkManager.mainFrame;
        if (!mainFrame)
            return [];

        let resolvedBreakpoints = [];
        let frames = [mainFrame];
        while (frames.length) {
            let frame = frames.shift();

            let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frame.id);
            if (domBreakpointNodeIdentifierMap)
                resolvedBreakpoints.pushAll(domBreakpointNodeIdentifierMap.values());

            frames.pushAll(frame.childFrameCollection);
        }

        return resolvedBreakpoints;
    }

    get listenerBreakpoints() { return this._listenerBreakpoints; }
    get urlBreakpoints() { return this._urlBreakpoints; }

    isBreakpointSpecial(breakpoint)
    {
        return breakpoint === this._allAnimationFramesBreakpoint
            || breakpoint === this._allIntervalsBreakpoint
            || breakpoint === this._allListenersBreakpoint
            || breakpoint === this._allTimeoutsBreakpoint
            || breakpoint === this._allRequestsBreakpoint;
    }

    domBreakpointsForNode(node)
    {
        console.assert(node instanceof WI.DOMNode);

        if (!node || !node.frame)
            return [];

        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(node.frame.id);
        if (!domBreakpointNodeIdentifierMap)
            return [];

        let breakpoints = domBreakpointNodeIdentifierMap.get(node.id);
        return breakpoints ? Array.from(breakpoints) : [];
    }

    domBreakpointsInSubtree(node)
    {
        console.assert(node instanceof WI.DOMNode);

        let breakpoints = [];

        if (node.children) {
            let children = Array.from(node.children);
            while (children.length) {
                let child = children.pop();
                if (child.children)
                    children.pushAll(child.children);
                breakpoints.pushAll(this.domBreakpointsForNode(child));
            }
        }

        return breakpoints;
    }

    addDOMBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.DOMBreakpoint);
        if (!breakpoint || !breakpoint.url)
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.DOMBreakpointAdded, {breakpoint});
            return;
        }

        this._domBreakpointURLMap.add(breakpoint.url, breakpoint);

        if (breakpoint.domNodeIdentifier)
            this._resolveDOMBreakpoint(breakpoint, breakpoint.domNodeIdentifier);

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.DOMBreakpointAdded, {breakpoint});

        if (!this._restoringBreakpoints)
            WI.objectStores.domBreakpoints.putObject(breakpoint);
    }

    removeDOMBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.DOMBreakpoint);
        if (!breakpoint)
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            breakpoint.disabled = true;
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.DOMBreakpointRemoved, {breakpoint});
            return;
        }

        this._detachDOMBreakpoint(breakpoint);

        this._domBreakpointURLMap.delete(breakpoint.url);

        if (!breakpoint.disabled) {
            // We should get the target associated with the nodeIdentifier of this breakpoint.
            let target = WI.assumingMainTarget();
            target.DOMDebuggerAgent.removeDOMBreakpoint(breakpoint.domNodeIdentifier, breakpoint.type);
        }

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.DOMBreakpointRemoved, {breakpoint});

        breakpoint.domNodeIdentifier = null;

        if (!this._restoringBreakpoints)
            WI.objectStores.domBreakpoints.deleteObject(breakpoint);
    }

    removeDOMBreakpointsForNode(node)
    {
        this.domBreakpointsForNode(node).forEach(this.removeDOMBreakpoint, this);
    }

    listenerBreakpointForEventName(eventName)
    {
        if (DOMDebuggerManager.supportsAllListenersBreakpoint() && !this._allListenersBreakpoint.disabled)
            return this._allListenersBreakpoint;
        return this._listenerBreakpoints.find((breakpoint) => breakpoint.eventName === eventName) || null;
    }

    addEventBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.EventBreakpoint, breakpoint);
        if (!breakpoint)
            return false;

        if (this.isBreakpointSpecial(breakpoint)) {
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.EventBreakpointAdded, {breakpoint});
            return true;
        }

        console.assert(breakpoint.type === WI.EventBreakpoint.Type.Listener, breakpoint);
        console.assert(breakpoint.eventName, breakpoint);

        if (this._listenerBreakpoints.find((existing) => existing.eventName === breakpoint.eventName))
            return false;

        this._listenerBreakpoints.push(breakpoint);

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.EventBreakpointAdded, {breakpoint});

        if (!breakpoint.disabled) {
            for (let target of WI.targets)
                this._updateEventBreakpoint(breakpoint, target);
        }

        if (!this._restoringBreakpoints)
            WI.objectStores.eventBreakpoints.putObject(breakpoint);

        return true;
    }

    removeEventBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.EventBreakpoint, breakpoint);
        if (!breakpoint)
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            breakpoint.disabled = true;
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.EventBreakpointRemoved, {breakpoint});
            return;
        }

        console.assert(breakpoint.type === WI.EventBreakpoint.Type.Listener, breakpoint);
        console.assert(breakpoint.eventName, breakpoint);

        console.assert(this._listenerBreakpoints.includes(breakpoint), breakpoint);
        if (!this._listenerBreakpoints.includes(breakpoint))
            return;

        this._listenerBreakpoints.remove(breakpoint);

        if (!this._restoringBreakpoints)
            WI.objectStores.eventBreakpoints.deleteObject(breakpoint);

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.EventBreakpointRemoved, {breakpoint});

        if (breakpoint.disabled)
            return;

        for (let target of WI.targets) {
            // COMPATIBILITY (iOS 12): DOMDebugger.removeEventBreakpoint did not exist.
            if (target.hasCommand("DOMDebugger.removeEventBreakpoint"))
                target.DOMDebuggerAgent.removeEventBreakpoint(breakpoint.type, breakpoint.eventName);
            else if (target.hasCommand("DOMDebugger.removeEventListenerBreakpoint")) {
                console.assert(breakpoint.type === WI.EventBreakpoint.Type.Listener);
                target.DOMDebuggerAgent.removeEventListenerBreakpoint(breakpoint.eventName);
            }
        }
    }

    urlBreakpointForURL(url)
    {
        return this._urlBreakpoints.find((breakpoint) => breakpoint.url === url) || null;
    }

    addURLBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.URLBreakpoint);
        if (!breakpoint)
            return false;

        if (this.isBreakpointSpecial(breakpoint)) {
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.URLBreakpointAdded, {breakpoint});
            return true;
        }

        console.assert(!this._urlBreakpoints.includes(breakpoint), "Already added URL breakpoint.", breakpoint);
        if (this._urlBreakpoints.includes(breakpoint))
            return false;

        if (this._urlBreakpoints.some((entry) => entry.type === breakpoint.type && entry.url === breakpoint.url))
            return false;

        this._urlBreakpoints.push(breakpoint);

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.URLBreakpointAdded, {breakpoint});

        if (!breakpoint.disabled) {
            for (let target of WI.targets)
                this._updateURLBreakpoint(breakpoint, target);
        }

        if (!this._restoringBreakpoints)
            WI.objectStores.urlBreakpoints.putObject(breakpoint);

        return true;
    }

    removeURLBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.URLBreakpoint);
        if (!breakpoint)
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            breakpoint.disabled = true;
            this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.URLBreakpointRemoved, {breakpoint});
            return;
        }

        if (!this._urlBreakpoints.includes(breakpoint))
            return;

        this._urlBreakpoints.remove(breakpoint, true);

        if (!this._restoringBreakpoints)
            WI.objectStores.urlBreakpoints.deleteObject(breakpoint);

        this.dispatchEventToListeners(WI.DOMDebuggerManager.Event.URLBreakpointRemoved, {breakpoint});

        if (breakpoint.disabled)
            return;

        for (let target of WI.targets) {
            // COMPATIBILITY (iOS 12.1): DOMDebugger.removeURLBreakpoint did not exist.
            if (target.hasCommand("DOMDebugger.removeURLBreakpoint"))
                target.DOMDebuggerAgent.removeURLBreakpoint(breakpoint.url);
            else if (target.hasCommand("DOMDebugger.removeXHRBreakpoint"))
                target.DOMDebuggerAgent.removeXHRBreakpoint(breakpoint.url);
        }
    }

    // Private

    _detachDOMBreakpoint(breakpoint)
    {
        let nodeIdentifier = breakpoint.domNodeIdentifier;
        let node = WI.domManager.nodeForId(nodeIdentifier);
        console.assert(node, "Missing DOM node for breakpoint.", breakpoint);
        if (!node || !node.frame)
            return;

        let frameIdentifier = node.frame.id;
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frameIdentifier);
        console.assert(domBreakpointNodeIdentifierMap, "Missing DOM breakpoints for node parent frame.", node);
        if (!domBreakpointNodeIdentifierMap)
            return;

        domBreakpointNodeIdentifierMap.delete(nodeIdentifier, breakpoint);

        if (!domBreakpointNodeIdentifierMap.size)
            this._domBreakpointFrameIdentifierMap.delete(frameIdentifier);
    }

    _detachBreakpointsForFrame(frame)
    {
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frame.id);
        if (!domBreakpointNodeIdentifierMap)
            return;

        this._domBreakpointFrameIdentifierMap.delete(frame.id);

        for (let breakpoint of domBreakpointNodeIdentifierMap.values())
            breakpoint.domNodeIdentifier = null;
    }

    _speculativelyResolveDOMBreakpointsForURL(url)
    {
        let domBreakpoints = this._domBreakpointURLMap.get(url);
        if (!domBreakpoints)
            return;

        for (let breakpoint of domBreakpoints) {
            if (breakpoint.domNodeIdentifier)
                continue;

            WI.domManager.pushNodeByPathToFrontend(breakpoint.path, (nodeIdentifier) => {
                if (breakpoint.domNodeIdentifier) {
                    // This breakpoint may have been resolved by a node being inserted before this
                    // callback is invoked.  If so, the `nodeIdentifier` should match, so don't try
                    // to resolve it again as it would've already been resolved.
                    console.assert(breakpoint.domNodeIdentifier === nodeIdentifier);
                    return;
                }

                if (!nodeIdentifier)
                    return;

                this._restoringBreakpoints = true;
                this._resolveDOMBreakpoint(breakpoint, nodeIdentifier);
                this._restoringBreakpoints = false;
            });
        }
    }

    _resolveDOMBreakpoint(breakpoint, nodeIdentifier)
    {
        let node = WI.domManager.nodeForId(nodeIdentifier);
        console.assert(node, "Missing DOM node for nodeIdentifier.", nodeIdentifier);
        if (!node || !node.frame)
            return;

        let frameIdentifier = node.frame.id;
        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(frameIdentifier);
        if (!domBreakpointNodeIdentifierMap) {
            domBreakpointNodeIdentifierMap = new Multimap;
            this._domBreakpointFrameIdentifierMap.set(frameIdentifier, domBreakpointNodeIdentifierMap);
        }

        domBreakpointNodeIdentifierMap.add(nodeIdentifier, breakpoint);

        breakpoint.domNodeIdentifier = nodeIdentifier;

        if (!breakpoint.disabled) {
            // We should get the target associated with the nodeIdentifier of this breakpoint.
            let target = WI.assumingMainTarget();
            if (target)
                this._updateDOMBreakpoint(breakpoint, target);
        }
    }

    _updateDOMBreakpoint(breakpoint, target)
    {
        console.assert(target.type !== WI.TargetType.Worker, "Worker targets do not support DOM breakpoints");
        if (target.type === WI.TargetType.Worker)
            return;

        if (!target.hasCommand("DOMDebugger.setDOMBreakpoint") || !target.hasCommand("DOMDebugger.removeDOMBreakpoint"))
            return;

        if (!breakpoint.domNodeIdentifier)
            return;

        if (breakpoint.disabled)
            target.DOMDebuggerAgent.removeDOMBreakpoint(breakpoint.domNodeIdentifier, breakpoint.type);
        else {
            if (!this._restoringBreakpoints && !WI.debuggerManager.breakpointsDisabledTemporarily)
                WI.debuggerManager.breakpointsEnabled = true;

            target.DOMDebuggerAgent.setDOMBreakpoint(breakpoint.domNodeIdentifier, breakpoint.type);
        }
    }

    _updateEventBreakpoint(breakpoint, target)
    {
        // Worker targets do not support `requestAnimationFrame` breakpoints.
        if (breakpoint === this._allAnimationFramesBreakpoint && target.type === WI.TargetType.Worker)
            return;

        // COMPATIBILITY (iOS 12): DOMDebugger.removeEventBreakpoint did not exist.
        if (target.hasCommand("DOMDebugger.setEventListenerBreakpoint") && target.hasCommand("DOMDebugger.removeEventListenerBreakpoint")) {
            console.assert(breakpoint.type === WI.EventBreakpoint.Type.Listener);
            if (breakpoint.disabled)
                target.DOMDebuggerAgent.removeEventListenerBreakpoint(breakpoint.eventName);
            else {
                if (!this._restoringBreakpoints && !WI.debuggerManager.breakpointsDisabledTemporarily)
                    WI.debuggerManager.breakpointsEnabled = true;

                target.DOMDebuggerAgent.setEventListenerBreakpoint(breakpoint.eventName);
            }
            return;
        }

        if (!target.hasCommand("DOMDebugger.setEventBreakpoint") || !target.hasCommand("DOMDebugger.removeEventBreakpoint"))
            return;

        let commandArguments = {};

        switch (breakpoint) {
        case this._allAnimationFramesBreakpoint:
            commandArguments.breakpointType = WI.EventBreakpoint.Type.AnimationFrame;
            if (!DOMDebuggerManager.supportsAllListenersBreakpoint())
                commandArguments.eventName = "requestAnimationFrame";
            break;

        case this._allIntervalsBreakpoint:
            if (DOMDebuggerManager.supportsAllListenersBreakpoint())
                commandArguments.breakpointType = WI.EventBreakpoint.Type.Interval;
            else {
                commandArguments.breakpointType = WI.EventBreakpoint.Type.Timer;
                commandArguments.eventName = "setInterval";
            }
            break;

        case this._allListenersBreakpoint:
            if (!DOMDebuggerManager.supportsAllListenersBreakpoint())
                return;

            commandArguments.breakpointType = WI.EventBreakpoint.Type.Listener;
            break;

        case this._allTimeoutsBreakpoint:
            if (DOMDebuggerManager.supportsAllListenersBreakpoint())
                commandArguments.breakpointType = WI.EventBreakpoint.Type.Timeout;
            else {
                commandArguments.breakpointType = WI.EventBreakpoint.Type.Timer;
                commandArguments.eventName = "setTimeout";
            }
            break;

        default:
            commandArguments.breakpointType = breakpoint.type;
            commandArguments.eventName = breakpoint.eventName;
            console.assert(commandArguments.eventName);
            break;
        }

        const callback = null;

        if (breakpoint.disabled)
            target.DOMDebuggerAgent.removeEventBreakpoint.invoke(commandArguments, callback);
        else {
            if (!this._restoringBreakpoints && !WI.debuggerManager.breakpointsDisabledTemporarily)
                WI.debuggerManager.breakpointsEnabled = true;

            target.DOMDebuggerAgent.setEventBreakpoint.invoke(commandArguments, callback);
        }
    }

    _updateURLBreakpoint(breakpoint, target)
    {
        // COMPATIBILITY (iOS 12.1): DOMDebugger.removeURLBreakpoint did not exist.
        if (target.hasCommand("DOMDebugger.setXHRBreakpoint") && target.hasCommand("DOMDebugger.removeXHRBreakpoint")) {
            if (breakpoint.disabled)
                target.DOMDebuggerAgent.removeXHRBreakpoint(breakpoint.url);
            else {
                if (!this._restoringBreakpoints && !WI.debuggerManager.breakpointsDisabledTemporarily)
                    WI.debuggerManager.breakpointsEnabled = true;

                let isRegex = breakpoint.type === WI.URLBreakpoint.Type.RegularExpression;
                target.DOMDebuggerAgent.setXHRBreakpoint(breakpoint.url, isRegex);
            }
            return;
        }

        if (!target.hasCommand("DOMDebugger.setURLBreakpoint") || !target.hasCommand("DOMDebugger.removeURLBreakpoint"))
            return;

        if (breakpoint.disabled)
            target.DOMDebuggerAgent.removeURLBreakpoint(breakpoint.url);
        else {
            if (!this._restoringBreakpoints && !WI.debuggerManager.breakpointsDisabledTemporarily)
                WI.debuggerManager.breakpointsEnabled = true;

            let isRegex = breakpoint.type === WI.URLBreakpoint.Type.RegularExpression;
            target.DOMDebuggerAgent.setURLBreakpoint(breakpoint.url, isRegex);
        }
    }

    _handleDOMBreakpointDisabledStateChanged(event)
    {
        let breakpoint = event.target;
        let target = WI.assumingMainTarget();
        if (target)
            this._updateDOMBreakpoint(breakpoint, target);

        if (!this._restoringBreakpoints)
            WI.objectStores.domBreakpoints.putObject(breakpoint);
    }

    _handleEventBreakpointDisabledStateChanged(event)
    {
        let breakpoint = event.target;

        // Specific event listener breakpoints are handled by `DOMManager`.
        if (breakpoint.eventListener)
            return;

        for (let target of WI.targets)
            this._updateEventBreakpoint(breakpoint, target);

        switch (breakpoint) {
        case this._allAnimationFramesBreakpoint:
            this._allAnimationFramesBreakpointEnabledSetting.value = !breakpoint.disabled;
            return;

        case this._allIntervalsBreakpoint:
            this._allIntervalsBreakpointEnabledSetting.value = !breakpoint.disabled;
            return;

        case this._allListenersBreakpoint:
            this._allListenersBreakpointEnabledSetting.value = !breakpoint.disabled;
            return;

        case this._allTimeoutsBreakpoint:
            this._allTimeoutsBreakpointEnabledSetting.value = !breakpoint.disabled;
            return;
        }

        if (!this._restoringBreakpoints)
            WI.objectStores.eventBreakpoints.putObject(breakpoint);
    }

    _handleURLBreakpointDisabledStateChanged(event)
    {
        let breakpoint = event.target;

        for (let target of WI.targets)
            this._updateURLBreakpoint(breakpoint, target);

        if (breakpoint === this._allRequestsBreakpoint) {
            this._allRequestsBreakpointEnabledSetting.value = !breakpoint.disabled;
            return;
        }

        if (!this._restoringBreakpoints)
            WI.objectStores.urlBreakpoints.putObject(breakpoint);
    }

    _childFrameWasRemoved(event)
    {
        let frame = event.data.childFrame;
        this._detachBreakpointsForFrame(frame);
    }

    _mainFrameDidChange(event)
    {
        this._speculativelyResolveDOMBreakpointsForURL(WI.networkManager.mainFrame.url);
    }

    _mainResourceDidChange(event)
    {
        let frame = event.target;
        if (frame.isMainFrame()) {
            for (let breakpoint of this._domBreakpointURLMap.values())
                breakpoint.domNodeIdentifier = null;

            this._domBreakpointFrameIdentifierMap.clear();
        } else
            this._detachBreakpointsForFrame(frame);

        this._speculativelyResolveDOMBreakpointsForURL(frame.url);
    }

    _nodeInserted(event)
    {
        let node = event.data.node;
        if (node.nodeType() !== Node.ELEMENT_NODE || !node.frame)
            return;

        let url = node.frame.url;
        let breakpoints = this._domBreakpointURLMap.get(url);
        if (!breakpoints)
            return;

        for (let breakpoint of breakpoints) {
            if (breakpoint.domNodeIdentifier)
                continue;

            if (breakpoint.path !== node.path())
                continue;

            this._restoringBreakpoints = true;
            this._resolveDOMBreakpoint(breakpoint, node.id);
            this._restoringBreakpoints = false;
        }
    }

    _nodeRemoved(event)
    {
        let node = event.data.node;
        if (node.nodeType() !== Node.ELEMENT_NODE || !node.frame)
            return;

        let domBreakpointNodeIdentifierMap = this._domBreakpointFrameIdentifierMap.get(node.frame.id);
        if (!domBreakpointNodeIdentifierMap)
            return;

        let breakpoints = domBreakpointNodeIdentifierMap.get(node.id);
        if (!breakpoints)
            return;

        domBreakpointNodeIdentifierMap.delete(node.id);

        if (!domBreakpointNodeIdentifierMap.size)
            this._domBreakpointFrameIdentifierMap.delete(node.frame.id);

        for (let breakpoint of breakpoints)
            breakpoint.domNodeIdentifier = null;
    }
};

WI.DOMDebuggerManager.Event = {
    DOMBreakpointAdded: "dom-debugger-manager-dom-breakpoint-added",
    DOMBreakpointRemoved: "dom-debugger-manager-dom-breakpoint-removed",
    EventBreakpointAdded: "dom-debugger-manager-event-breakpoint-added",
    EventBreakpointRemoved: "dom-debugger-manager-event-breakpoint-removed",
    URLBreakpointAdded: "dom-debugger-manager-url-breakpoint-added",
    URLBreakpointRemoved: "dom-debugger-manager-url-breakpoint-removed",
};
