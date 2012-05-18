/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

/**
 * @constructor
 * @param {number=} size
 */
WebInspector.Uint32Array = function(size)
{
    const preallocateSize = 1000;
    size = size || preallocateSize;
    this._usedSize = 0;
    this._array = new Uint32Array(preallocateSize);
}

WebInspector.Uint32Array.prototype = {
    push: function(value)
    {
        if (this._usedSize + 1 > this._array.length) {
            var tempArray = new Uint32Array(this._array.length * 2);
            tempArray.set(this._array);
            this._array = tempArray;
        }
        this._array[this._usedSize++] = value;
    },

    get array()
    {
        return this._array.subarray(0, this._usedSize);
    }
}

/**
 * @constructor
 */
WebInspector.HeapSnapshotArraySlice = function(array, start, end)
{
    this._array = array;
    this._start = start;
    this.length = end - start;
}

WebInspector.HeapSnapshotArraySlice.prototype = {
    item: function(index)
    {
        return this._array[this._start + index];
    },

    slice: function(start, end)
    {
        if (typeof end === "undefined")
            end = this.length;
        return this._array.subarray(this._start + start, this._start + end);
    }
}

/**
 * @constructor
 * @param {number=} edgeIndex
 */
WebInspector.HeapSnapshotEdge = function(snapshot, edges, edgeIndex)
{
    this._snapshot = snapshot;
    this._edges = edges;
    this.edgeIndex = edgeIndex || 0;
}

WebInspector.HeapSnapshotEdge.prototype = {
    clone: function()
    {
        return new WebInspector.HeapSnapshotEdge(this._snapshot, this._edges, this.edgeIndex);
    },

    get hasStringName()
    {
        if (!this.isShortcut)
            return this._hasStringName;
        return isNaN(parseInt(this._name, 10));
    },

    get isElement()
    {
        return this._type() === this._snapshot._edgeElementType;
    },

    get isHidden()
    {
        return this._type() === this._snapshot._edgeHiddenType;
    },

    get isWeak()
    {
        return this._type() === this._snapshot._edgeWeakType;
    },

    get isInternal()
    {
        return this._type() === this._snapshot._edgeInternalType;
    },

    get isInvisible()
    {
        return this._type() === this._snapshot._edgeInvisibleType;
    },

    get isShortcut()
    {
        return this._type() === this._snapshot._edgeShortcutType;
    },

    get name()
    {
        if (!this.isShortcut)
            return this._name;
        var numName = parseInt(this._name, 10);
        return isNaN(numName) ? this._name : numName;
    },

    get node()
    {
        return new WebInspector.HeapSnapshotNode(this._snapshot, this.nodeIndex);
    },

    get nodeIndex()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeToNodeOffset);
    },

    get rawEdges()
    {
        return this._edges;
    },

    toString: function()
    {
        switch (this.type) {
        case "context": return "->" + this.name;
        case "element": return "[" + this.name + "]";
        case "weak": return "[[" + this.name + "]]";
        case "property":
            return this.name.indexOf(" ") === -1 ? "." + this.name : "[\"" + this.name + "\"]";
        case "shortcut":
            var name = this.name;
            if (typeof name === "string")
                return this.name.indexOf(" ") === -1 ? "." + this.name : "[\"" + this.name + "\"]";
            else
                return "[" + this.name + "]";
        case "internal":
        case "hidden":
        case "invisible":
            return "{" + this.name + "}";
        };
        return "?" + this.name + "?";
    },

    get type()
    {
        return this._snapshot._edgeTypes[this._type()];
    },

    get _hasStringName()
    {
        return !this.isElement && !this.isHidden && !this.isWeak;
    },

    get _name()
    {
        return this._hasStringName ? this._snapshot._strings[this._nameOrIndex] : this._nameOrIndex;
    },

    get _nameOrIndex()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeNameOffset);
    },

    _type: function()
    {
        return this._edges.item(this.edgeIndex + this._snapshot._edgeTypeOffset);
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotEdgeIterator = function(edge)
{
    this.edge = edge;
}

WebInspector.HeapSnapshotEdgeIterator.prototype = {
    first: function()
    {
        this.edge.edgeIndex = 0;
    },

    hasNext: function()
    {
        return this.edge.edgeIndex < this.edge._edges.length;
    },

    get index()
    {
        return this.edge.edgeIndex;
    },

    set index(newIndex)
    {
        this.edge.edgeIndex = newIndex;
    },

    get item()
    {
        return this.edge;
    },

    next: function()
    {
        this.edge.edgeIndex += this.edge._snapshot._edgeFieldsCount;
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotRetainerEdge = function(snapshot, retainedNodeIndex, retainerIndex)
{
    this._snapshot = snapshot;
    this._retainedNodeIndex = retainedNodeIndex;

    var retainedNodeOrdinal = retainedNodeIndex / snapshot._nodeFieldCount;
    this._firstRetainer = snapshot._firstRetainerIndex[retainedNodeOrdinal];
    this._retainersCount = snapshot._firstRetainerIndex[retainedNodeOrdinal + 1] - this._firstRetainer;

    this.retainerIndex = retainerIndex;
}

WebInspector.HeapSnapshotRetainerEdge.prototype = {
    clone: function()
    {
        return new WebInspector.HeapSnapshotRetainerEdge(this._snapshot, this._retainedNodeIndex, this.retainerIndex);
    },

    get hasStringName()
    {
        return this._edge.hasStringName;
    },

    get isElement()
    {
        return this._edge.isElement;
    },

    get isHidden()
    {
        return this._edge.isHidden;
    },

    get isInternal()
    {
        return this._edge.isInternal;
    },

    get isInvisible()
    {
        return this._edge.isInvisible;
    },

    get isShortcut()
    {
        return this._edge.isShortcut;
    },

    get isWeak()
    {
        return this._edge.isWeak;
    },

    get name()
    {
        return this._edge.name;
    },

    get node()
    {
        return this._node;
    },

    get nodeIndex()
    {
        return this._nodeIndex;
    },

    get retainerIndex()
    {
        return this._retainerIndex;
    },

    set retainerIndex(newIndex)
    {
        if (newIndex !== this._retainerIndex) {
            this._retainerIndex = newIndex;
            this.edgeIndex = newIndex;
        }
    },

    set edgeIndex(edgeIndex)
    {
        var retainerIndex = this._firstRetainer + edgeIndex;
        this._globalEdgeIndex = this._snapshot._retainingEdges[retainerIndex];
        this._nodeIndex = this._snapshot._retainingNodes[retainerIndex];
        delete this._edgeInstance;
        delete this._nodeInstance;
    },

    get _node()
    {
        if (!this._nodeInstance)
            this._nodeInstance = new WebInspector.HeapSnapshotNode(this._snapshot, this._nodeIndex);
        return this._nodeInstance;
    },

    get _edge()
    {
        if (!this._edgeInstance) {
            var edgeIndex = this._globalEdgeIndex - this._node._edgeIndexesStart();
            this._edgeInstance = new WebInspector.HeapSnapshotEdge(this._snapshot, this._node.rawEdges, edgeIndex);
        }
        return this._edgeInstance;
    },

    toString: function()
    {
        return this._edge.toString();
    },

    get type()
    {
        return this._edge.type;
    }
}

/**
 * @constructor
 */
WebInspector.HeapSnapshotRetainerEdgeIterator = function(retainer)
{
    this.retainer = retainer;
}

WebInspector.HeapSnapshotRetainerEdgeIterator.prototype = {
    first: function()
    {
        this.retainer.retainerIndex = 0;
    },

    hasNext: function()
    {
        return this.retainer.retainerIndex < this.retainer._retainersCount;
    },

    get index()
    {
        return this.retainer.retainerIndex;
    },

    set index(newIndex)
    {
        this.retainer.retainerIndex = newIndex;
    },

    get item()
    {
        return this.retainer;
    },

    next: function()
    {
        ++this.retainer.retainerIndex;
    }
};

/**
 * @constructor
 * @param {number=} nodeIndex
 */
WebInspector.HeapSnapshotNode = function(snapshot, nodeIndex)
{
    this._snapshot = snapshot;
    this._firstNodeIndex = nodeIndex;
    this.nodeIndex = nodeIndex;
}

WebInspector.HeapSnapshotNode.prototype = {
    get canBeQueried()
    {
        var flags = this._snapshot._flagsOfNode(this);
        return !!(flags & this._snapshot._nodeFlags.canBeQueried);
    },

    get distanceToWindow()
    {
        return this._snapshot._distancesToWindow[this.nodeIndex / this._snapshot._nodeFieldCount];
    },

    get className()
    {
        switch (this.type) {
        case "hidden":
            return WebInspector.UIString("(system)");
        case "object":
        case "native":
            return this.name;
        case "code":
            return WebInspector.UIString("(compiled code)");
        default:
            return "(" + this.type + ")";
        }
    },

    get classIndex()
    {
        var type = this._type();
        switch (type) {
        case this._snapshot._nodeObjectType:
        case this._snapshot._nodeNativeType:
            return this._name();
        default:
            return -1 - type;
        }
    },

    get dominatorIndex()
    {
        return this._nodes[this.nodeIndex + this._snapshot._dominatorOffset];
    },

    get edges()
    {
        return new WebInspector.HeapSnapshotEdgeIterator(new WebInspector.HeapSnapshotEdge(this._snapshot, this.rawEdges));
    },

    get edgesCount()
    {
        return (this._edgeIndexesEnd() - this._edgeIndexesStart()) / this._snapshot._edgeFieldsCount;
    },

    get flags()
    {
        return this._snapshot._flagsOfNode(this);
    },

    get id()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeIdOffset];
    },

    get isHidden()
    {
        return this._type() === this._snapshot._nodeHiddenType;
    },

    get isNative()
    {
        return this._type() === this._snapshot._nodeNativeType;
    },

    get isSynthetic()
    {
        return this._type() === this._snapshot._nodeSyntheticType;
    },

    get isWindow()
    {
        const windowRE = /^Window/;
        return windowRE.test(this.name);
    },

    get isDetachedDOMTreesRoot()
    {
        return this.name === "(Detached DOM trees)";
    },

    get isDetachedDOMTree()
    {
        const detachedDOMTreeRE = /^Detached DOM tree/;
        return detachedDOMTreeRE.test(this.className);
    },

    get isRoot()
    {
        return this.nodeIndex === this._snapshot._rootNodeIndex;
    },

    get name()
    {
        return this._snapshot._strings[this._name()];
    },

    get rawEdges()
    {
        return new WebInspector.HeapSnapshotArraySlice(this._snapshot._containmentEdges, this._edgeIndexesStart(), this._edgeIndexesEnd());
    },

    get retainedSize()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeRetainedSizeOffset];
    },

    get retainers()
    {
        return new WebInspector.HeapSnapshotRetainerEdgeIterator(new WebInspector.HeapSnapshotRetainerEdge(this._snapshot, this.nodeIndex, 0));
    },

    get selfSize()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeSelfSizeOffset];
    },

    get type()
    {
        return this._snapshot._nodeTypes[this._type()];
    },

    _name: function()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeNameOffset];
    },

    get _nodes()
    {
        return this._snapshot._nodes;
    },

    _edgeIndexesStart: function()
    {
        return this._snapshot._nodes[this.nodeIndex + this._snapshot._firstEdgeIndexOffset];
    },

    _edgeIndexesEnd: function()
    {
        var nextNodeIndex = this._nextNodeIndex;
        if (nextNodeIndex < this._snapshot._nodes.length)
            return this._snapshot._nodes[nextNodeIndex + this._snapshot._firstEdgeIndexOffset]
        return this._snapshot._containmentEdges.length;
    },

    get _nextNodeIndex()
    {
        return this.nodeIndex + this._snapshot._nodeFieldCount;
    },

    _type: function()
    {
        return this._nodes[this.nodeIndex + this._snapshot._nodeTypeOffset];
    }
};

/**
 * @constructor
 */
WebInspector.HeapSnapshotNodeIterator = function(node)
{
    this.node = node;
}

WebInspector.HeapSnapshotNodeIterator.prototype = {
    first: function()
    {
        this.node.nodeIndex = this.node._firstNodeIndex;
    },

    hasNext: function()
    {
        return this.node.nodeIndex < this.node._nodes.length;
    },

    get index()
    {
        return this.node.nodeIndex;
    },

    set index(newIndex)
    {
        this.node.nodeIndex = newIndex;
    },

    get item()
    {
        return this.node;
    },

    next: function()
    {
        this.node.nodeIndex = this.node._nextNodeIndex;
    }
}

/**
 * @constructor
 */
WebInspector.HeapSnapshot = function(profile)
{
    this.uid = profile.snapshot.uid;
    this._nodes = profile.nodes;
    this._containmentEdges = profile.edges;
    /** @type{HeapSnapshotMetainfo} */
    this._metaNode = profile.snapshot.meta;
    this._strings = profile.strings;

    this._snapshotDiffs = {};
    this._aggregatesForDiff = null;

    this._init();
}

/**
 * @constructor
 */
function HeapSnapshotMetainfo()
{
    // New format.
    this.node_fields = [];
    this.node_types = [];
    this.edge_fields = [];
    this.edge_types = [];

    // Old format.
    this.fields = [];
    this.types = [];
}

/**
 * @constructor
 */
function HeapSnapshotHeader()
{
    // New format.
    this.title = "";
    this.uid = 0;
    this.meta = new HeapSnapshotMetainfo();
    this.node_count = 0;
    this.edge_count = 0;
}

WebInspector.HeapSnapshot.prototype = {
    _init: function()
    {
        var meta = this._metaNode;
        this._rootNodeIndex = 0;

        this._nodeTypeOffset = meta.node_fields.indexOf("type");
        this._nodeNameOffset = meta.node_fields.indexOf("name");
        this._nodeIdOffset = meta.node_fields.indexOf("id");
        this._nodeSelfSizeOffset = meta.node_fields.indexOf("self_size");
        this._nodeRetainedSizeOffset = meta.node_fields.indexOf("retained_size");
        this._dominatorOffset = meta.node_fields.indexOf("dominator");
        this._firstEdgeIndexOffset = meta.node_fields.indexOf("edges_index");
        this._nodeFieldCount = meta.node_fields.length;

        this._nodeTypes = meta.node_types[this._nodeTypeOffset];
        this._nodeHiddenType = this._nodeTypes.indexOf("hidden");
        this._nodeObjectType = this._nodeTypes.indexOf("object");
        this._nodeNativeType = this._nodeTypes.indexOf("native");
        this._nodeCodeType = this._nodeTypes.indexOf("code");
        this._nodeSyntheticType = this._nodeTypes.indexOf("synthetic");

        this._edgeFieldsCount = meta.edge_fields.length;
        this._edgeTypeOffset = meta.edge_fields.indexOf("type");
        this._edgeNameOffset = meta.edge_fields.indexOf("name_or_index");
        this._edgeToNodeOffset = meta.edge_fields.indexOf("to_node");

        this._edgeTypes = meta.edge_types[this._edgeTypeOffset];
        this._edgeTypes.push("invisible");
        this._edgeElementType = this._edgeTypes.indexOf("element");
        this._edgeHiddenType = this._edgeTypes.indexOf("hidden");
        this._edgeInternalType = this._edgeTypes.indexOf("internal");
        this._edgeShortcutType = this._edgeTypes.indexOf("shortcut");
        this._edgeWeakType = this._edgeTypes.indexOf("weak");
        this._edgeInvisibleType = this._edgeTypes.indexOf("invisible");

        this._nodeFlags = { // bit flags
            canBeQueried: 1,
            detachedDOMTreeNode: 2,
        };

        this.nodeCount = this._nodes.length / this._nodeFieldCount;
        this._edgeCount = this._containmentEdges.length / this._edgeFieldsCount;

        this._markInvisibleEdges();
        this._buildRetainers();
        if (this._dominatorOffset !== -1) // For tests where we may not have dominator field.
            this._buildDominatedNodes()
        this._calculateFlags();
        this._calculateObjectToWindowDistance();
    },

    _buildRetainers: function()
    {
        var retainingNodes = this._retainingNodes = new Uint32Array(this._edgeCount);
        var retainingEdges = this._retainingEdges = new Uint32Array(this._edgeCount);
        // Index of the first retainer in the _retainingNodes and _retainingEdges
        // arrays. Addressed by retained node index.
        var firstRetainerIndex = this._firstRetainerIndex = new Uint32Array(this.nodeCount + 1);

        var containmentEdges = this._containmentEdges;
        var edgeFieldsCount = this._edgeFieldsCount;
        var nodeFieldCount = this._nodeFieldCount;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var nodes = this._nodes;
        var firstEdgeIndexOffset = this._firstEdgeIndexOffset;

        for (var toNodeFieldIndex = edgeToNodeOffset, l = containmentEdges.length; toNodeFieldIndex < l; toNodeFieldIndex += edgeFieldsCount) {
            var toNodeIndex = containmentEdges[toNodeFieldIndex];
            if (toNodeIndex % nodeFieldCount)
                throw new Error("Invalid toNodeIndex " + toNodeIndex);
            ++firstRetainerIndex[toNodeIndex / nodeFieldCount];
        }
        for (var i = 0, firstUnusedRetainerSlot = 0, l = this.nodeCount; i < l; i++) {
            var retainersCount = firstRetainerIndex[i];
            firstRetainerIndex[i] = firstUnusedRetainerSlot;
            retainingNodes[firstUnusedRetainerSlot] = retainersCount;
            firstUnusedRetainerSlot += retainersCount;
        }
        firstRetainerIndex[this.nodeCount] = retainingNodes.length;

        var srcNodeIndex = 0;
        var nextNodeFirstEdgeIndex = nodes[firstEdgeIndexOffset];
        var nodesLength = nodes.length;
        while (srcNodeIndex < nodesLength) {
            var firstEdgeIndex = nextNodeFirstEdgeIndex;
            var nextNodeIndex = srcNodeIndex + nodeFieldCount;
            nextNodeFirstEdgeIndex = nextNodeIndex < nodesLength
                                   ? nodes[nextNodeIndex + firstEdgeIndexOffset]
                                   : containmentEdges.length;
            for (var edgeIndex = firstEdgeIndex; edgeIndex < nextNodeFirstEdgeIndex; edgeIndex += edgeFieldsCount) {
                var toNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                if (toNodeIndex % nodeFieldCount)
                    throw new Error("Invalid toNodeIndex " + toNodeIndex);
                var firstRetainerSlotIndex = firstRetainerIndex[toNodeIndex / nodeFieldCount];
                var nextUnusedRetainerSlotIndex = firstRetainerSlotIndex + (--retainingNodes[firstRetainerSlotIndex]);
                retainingNodes[nextUnusedRetainerSlotIndex] = srcNodeIndex;
                retainingEdges[nextUnusedRetainerSlotIndex] = edgeIndex;
            }
            srcNodeIndex = nextNodeIndex;
        }
    },

    dispose: function()
    {
        delete this._nodes;
        delete this._strings;
        delete this._retainingEdges;
        delete this._retainingNodes;
        delete this._firstRetainerIndex;
        if (this._aggregates) {
            delete this._aggregates;
            delete this._aggregatesSortedFlags;
        }
        delete this._dominatedNodes;
        delete this._firstDominatedNodeIndex;
        delete this._flags;
        delete this._distancesToWindow;
    },

    get _allNodes()
    {
        return new WebInspector.HeapSnapshotNodeIterator(this.rootNode);
    },

    get rootNode()
    {
        return new WebInspector.HeapSnapshotNode(this, this._rootNodeIndex);
    },

    get rootNodeIndex()
    {
        return this._rootNodeIndex;
    },

    get totalSize()
    {
        return this.rootNode.retainedSize;
    },

    _getDominatedIndex: function(nodeIndex)
    {
        if (nodeIndex % this._nodeFieldCount)
            throw new Error("Invalid nodeIndex: " + nodeIndex);
        return this._firstDominatedNodeIndex[nodeIndex / this._nodeFieldCount];
    },

    _dominatedNodesOfNode: function(node)
    {
        var dominatedIndexFrom = this._getDominatedIndex(node.nodeIndex);
        var dominatedIndexTo = this._getDominatedIndex(node._nextNodeIndex);
        return new WebInspector.HeapSnapshotArraySlice(this._dominatedNodes, dominatedIndexFrom, dominatedIndexTo);
    },

    _flagsOfNode: function(node)
    {
        return this._flags[node.nodeIndex / this._nodeFieldCount];
    },

    /**
     * @param {String=} filterString
     */
    aggregates: function(sortedIndexes, key, filterString)
    {
        if (!this._aggregates) {
            this._aggregates = {};
            this._aggregatesSortedFlags = {};
        }

        var aggregatesByClassName = this._aggregates[key];
        if (aggregatesByClassName) {
            if (sortedIndexes && !this._aggregatesSortedFlags[key]) {
                this._sortAggregateIndexes(aggregatesByClassName);
                this._aggregatesSortedFlags[key] = sortedIndexes;
            }
            return aggregatesByClassName;
        }

        var filter;
        if (filterString)
            filter = this._parseFilter(filterString);

        var aggregates = this._buildAggregates(filter);
        this._calculateClassesRetainedSize(aggregates.aggregatesByClassIndex, filter);
        aggregatesByClassName = aggregates.aggregatesByClassName;

        if (sortedIndexes)
            this._sortAggregateIndexes(aggregatesByClassName);

        this._aggregatesSortedFlags[key] = sortedIndexes;
        this._aggregates[key] = aggregatesByClassName;

        return aggregatesByClassName;
    },

    aggregatesForDiff: function()
    {
        if (this._aggregatesForDiff)
            return this._aggregatesForDiff;

        var aggregatesByClassName = this.aggregates(true, "allObjects");
        this._aggregatesForDiff  = {};

        var node = new WebInspector.HeapSnapshotNode(this);
        for (var className in aggregatesByClassName) {
            var aggregate = aggregatesByClassName[className];
            var indexes = aggregate.idxs;
            var ids = new Array(indexes.length);
            var selfSizes = new Array(indexes.length);
            for (var i = 0; i < indexes.length; i++) {
                node.nodeIndex = indexes[i];
                ids[i] = node.id;
                selfSizes[i] = node.selfSize;
            }

            this._aggregatesForDiff[className] = {
                indexes: indexes,
                ids: ids,
                selfSizes: selfSizes
            };
        }
        return this._aggregatesForDiff;
    },

    _calculateObjectToWindowDistance: function()
    {
        var nodeFieldCount = this._nodeFieldCount;
        var distances = new Uint32Array(this.nodeCount);

        // bfs for Window roots
        var list = [];
        for (var iter = this.rootNode.edges; iter.hasNext(); iter.next()) {
            var node = iter.edge.node;
            if (node.isWindow) {
                list.push(node.nodeIndex);
                distances[node.nodeIndex / nodeFieldCount] = 0;
            }
        }
        this._bfs(list, distances);

        // bfs for root
        list = [];
        list.push(this._rootNodeIndex);
        distances[this._rootNodeIndex / nodeFieldCount] = 0;
        this._bfs(list, distances);
        this._distancesToWindow = distances;
    },

    _bfs: function(list, distances)
    {
        // Peload fields into local variables for better performance.
        var edgeFieldsCount = this._edgeFieldsCount;
        var containmentEdges = this._containmentEdges;
        var nodeFieldCount = this._nodeFieldCount;
        var firstEdgeIndexOffset = this._firstEdgeIndexOffset;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;

        var index = 0;
        while (index < list.length) {
            var nodeIndex = list[index++]; // shift generates too much garbage.
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            if (index > 100000) {
                list = list.slice(index);
                index = 0;
            }
            var distance = distances[nodeOrdinal] + 1;
            var firstEdgeIndex = nodes[nodeIndex + firstEdgeIndexOffset];
            var edgesEnd = nodeOrdinal < nodeCount - 1
                         ? nodes[nodeIndex + firstEdgeIndexOffset + nodeFieldCount]
                         : containmentEdges.length;
            for (var edgeToNodeIndex = firstEdgeIndex + edgeToNodeOffset; edgeToNodeIndex < edgesEnd; edgeToNodeIndex += edgeFieldsCount) {
                var childNodeIndex = containmentEdges[edgeToNodeIndex];
                var childNodeOrdinal = childNodeIndex / nodeFieldCount;
                if (distances[childNodeOrdinal])
                    continue;
                distances[childNodeOrdinal] = distance;
                list.push(childNodeIndex);
            }
        }
    },

    _buildAggregates: function(filter)
    {
        var aggregates = {};
        var aggregatesByClassName = {};
        var nodes = this._nodes;
        var nodesLength = nodes.length;
        var nodeNativeType = this._nodeNativeType;
        var nodeFieldsCount = this._nodeFieldCount;
        var selfSizeOffset = this._nodeSelfSizeOffset;
        var nodeTypeOffset = this._nodeTypeOffset;
        var node = new WebInspector.HeapSnapshotNode(this, this._rootNodeIndex);
        var distancesToWindow = this._distancesToWindow;

        for (var nodeIndex = this._rootNodeIndex; nodeIndex < nodesLength; nodeIndex += nodeFieldsCount) {
            var nodeOrdinal = nodeIndex / nodeFieldsCount;
            node.nodeIndex = nodeIndex;
            var selfSize = nodes[nodeIndex + selfSizeOffset];
            if (filter && !filter(node))
                continue;
            if (!selfSize && nodes[nodeIndex + nodeTypeOffset] !== nodeNativeType)
                continue;
            var classIndex = node.classIndex;
            if (!(classIndex in aggregates)) {
                var nodeType = node.type;
                var nameMatters = nodeType === "object" || nodeType === "native";
                var value = {
                    count: 1,
                    distanceToWindow: distancesToWindow[nodeOrdinal],
                    self: selfSize,
                    maxRet: 0,
                    type: nodeType,
                    name: nameMatters ? node.name : null,
                    idxs: [nodeIndex]
                };
                aggregates[classIndex] = value;
                aggregatesByClassName[node.className] = value;
            } else {
                var clss = aggregates[classIndex];
                clss.distanceToWindow = Math.min(clss.distanceToWindow, distancesToWindow[nodeOrdinal]);
                ++clss.count;
                clss.self += selfSize;
                clss.idxs.push(nodeIndex);
            }
        }

        // Shave off provisionally allocated space.
        for (var classIndex in aggregates)
            aggregates[classIndex].idxs = aggregates[classIndex].idxs.slice(0);
        return {aggregatesByClassName: aggregatesByClassName, aggregatesByClassIndex: aggregates};
    },

    _calculateClassesRetainedSize: function(aggregates, filter)
    {
        var rootNodeIndex = this._rootNodeIndex;
        var node = new WebInspector.HeapSnapshotNode(this, rootNodeIndex);
        var list = [rootNodeIndex];
        var sizes = [-1];
        var classes = [];
        var seenClassNameIndexes = {};
        var nodeFieldCount = this._nodeFieldCount;
        var nodeTypeOffset = this._nodeTypeOffset;
        var nodeNativeType = this._nodeNativeType;
        var dominatedNodes = this._dominatedNodes;
        var nodes = this._nodes;
        var firstDominatedNodeIndex = this._firstDominatedNodeIndex;

        while (list.length) {
            var nodeIndex = list.pop();
            node.nodeIndex = nodeIndex;
            var classIndex = node.classIndex;
            var seen = !!seenClassNameIndexes[classIndex];
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            var dominatedIndexFrom = firstDominatedNodeIndex[nodeOrdinal];
            var dominatedIndexTo = firstDominatedNodeIndex[nodeOrdinal + 1];

            if (!seen &&
                (!filter || filter(node)) &&
                (node.selfSize || nodes[nodeIndex + nodeTypeOffset] === nodeNativeType)
               ) {
                aggregates[classIndex].maxRet += node.retainedSize;
                if (dominatedIndexFrom !== dominatedIndexTo) {
                    seenClassNameIndexes[classIndex] = true;
                    sizes.push(list.length);
                    classes.push(classIndex);
                }
            }
            for (var i = dominatedIndexFrom; i < dominatedIndexTo; i++)
                list.push(dominatedNodes[i]);

            var l = list.length;
            while (sizes[sizes.length - 1] === l) {
                sizes.pop();
                classIndex = classes.pop();
                seenClassNameIndexes[classIndex] = false;
            }
        }
    },

    _sortAggregateIndexes: function(aggregates)
    {
        var nodeA = new WebInspector.HeapSnapshotNode(this);
        var nodeB = new WebInspector.HeapSnapshotNode(this);
        for (var clss in aggregates)
            aggregates[clss].idxs.sort(
                function(idxA, idxB) {
                    nodeA.nodeIndex = idxA;
                    nodeB.nodeIndex = idxB;
                    return nodeA.id < nodeB.id ? -1 : 1;
                });
    },

    _buildDominatedNodes: function()
    {
        // Builds up two arrays:
        //  - "dominatedNodes" is a continuous array, where each node owns an
        //    interval (can be empty) with corresponding dominated nodes.
        //  - "indexArray" is an array of indexes in the "dominatedNodes"
        //    with the same positions as in the _nodeIndex.
        var indexArray = this._firstDominatedNodeIndex = new Uint32Array(this.nodeCount + 1);
        // All nodes except the root have dominators.
        var dominatedNodes = this._dominatedNodes = new Uint32Array(this.nodeCount - 1);

        // Count the number of dominated nodes for each node. Skip the root (node at
        // index 0) as it is the only node that dominates itself.
        for (var nodeIndex = this._nodeFieldCount; nodeIndex < this._nodes.length; nodeIndex += this._nodeFieldCount) {
            var dominatorIndex = this._nodes[nodeIndex + this._dominatorOffset];
            if (dominatorIndex % this._nodeFieldCount)
                throw new Error("Wrong dominatorIndex " + dominatorIndex + " nodeIndex = " + nodeIndex + " nodeCount = " + this.nodeCount);
            ++indexArray[dominatorIndex / this._nodeFieldCount];
        }
        // Put in the first slot of each dominatedNodes slice the count of entries
        // that will be filled.
        var firstDominatedNodeIndex = 0;
        for (var i = 0; i < this.nodeCount; ++i) {
            var dominatedCount = dominatedNodes[firstDominatedNodeIndex] = indexArray[i];
            indexArray[i] = firstDominatedNodeIndex;
            firstDominatedNodeIndex += dominatedCount;
        }
        indexArray[this.nodeCount] = dominatedNodes.length;
        // Fill up the dominatedNodes array with indexes of dominated nodes. Skip the root (node at
        // index 0) as it is the only node that dominates itself.
        for (var nodeIndex = this._nodeFieldCount; nodeIndex < this._nodes.length; nodeIndex += this._nodeFieldCount) {
            var dominatorIndex = this._nodes[nodeIndex + this._dominatorOffset];
            if (dominatorIndex % this._nodeFieldCount)
                throw new Error("Wrong dominatorIndex " + dominatorIndex);
            var dominatorPos = dominatorIndex / this._nodeFieldCount;
            var dominatedRefIndex = indexArray[dominatorPos];
            dominatedRefIndex += (--dominatedNodes[dominatedRefIndex]);
            dominatedNodes[dominatedRefIndex] = nodeIndex;
        }
    },

    _markInvisibleEdges: function()
    {
        // Mark hidden edges of global objects as invisible.
        // FIXME: This is a temporary measure. Normally, we should
        // really hide all hidden nodes.
        for (var iter = this.rootNode.edges; iter.hasNext(); iter.next()) {
            var edge = iter.edge;
            if (!edge.isShortcut)
                continue;
            var node = edge.node;
            var propNames = {};
            for (var innerIter = node.edges; innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (globalObjEdge.isShortcut)
                    propNames[globalObjEdge._nameOrIndex] = true;
            }
            for (innerIter.first(); innerIter.hasNext(); innerIter.next()) {
                var globalObjEdge = innerIter.edge;
                if (!globalObjEdge.isShortcut
                    && globalObjEdge.node.isHidden
                    && globalObjEdge._hasStringName
                    && (globalObjEdge._nameOrIndex in propNames))
                    this._containmentEdges[globalObjEdge._edges._start + globalObjEdge.edgeIndex + this._edgeTypeOffset] = this._edgeInvisibleType;
            }
        }
    },

    _numbersComparator: function(a, b)
    {
        return a < b ? -1 : (a > b ? 1 : 0);
    },

    _markDetachedDOMTreeNodes: function()
    {
        var flag = this._nodeFlags.detachedDOMTreeNode;
        var detachedDOMTreesRoot;
        for (var iter = this.rootNode.edges; iter.hasNext(); iter.next()) {
            var node = iter.edge.node;
            if (node.isDetachedDOMTreesRoot) {
                detachedDOMTreesRoot = node;
                break;
            }
        }

        if (!detachedDOMTreesRoot)
            return;

        for (var iter = detachedDOMTreesRoot.edges; iter.hasNext(); iter.next()) {
            var node = iter.edge.node;
            if (node.isDetachedDOMTree) {
                for (var edgesIter = node.edges; edgesIter.hasNext(); edgesIter.next())
                    this._flags[edgesIter.edge.node.nodeIndex / this._nodeFieldCount] |= flag;
            }
        }
    },

    _markQueriableHeapObjects: function()
    {
        // Allow runtime properties query for objects accessible from Window objects
        // via regular properties, and for DOM wrappers. Trying to access random objects
        // can cause a crash due to insonsistent state of internal properties of wrappers.
        var flag = this._nodeFlags.canBeQueried;
        var hiddenEdgeType = this._edgeHiddenType;
        var internalEdgeType = this._edgeInternalType;
        var invisibleEdgeType = this._edgeInvisibleType;
        var edgeToNodeOffset = this._edgeToNodeOffset;
        var edgeTypeOffset = this._edgeTypeOffset;
        var edgeFieldsCount = this._edgeFieldsCount;
        var containmentEdges = this._containmentEdges;
        var nodes = this._nodes;
        var nodeCount = this.nodeCount;
        var nodeFieldCount = this._nodeFieldCount;
        var firstEdgeIndexOffset = this._firstEdgeIndexOffset;

        var flags = this._flags;
        var list = [];
        for (var iter = this.rootNode.edges; iter.hasNext(); iter.next()) {
            if (iter.edge.node.isWindow)
                list.push(iter.edge.node.nodeIndex);
        }

        while (list.length) {
            var nodeIndex = list.pop();
            var nodeOrdinal = nodeIndex / nodeFieldCount;
            if (flags[nodeOrdinal] & flag)
                continue;
            flags[nodeOrdinal] |= flag;
            var beginEdgeIndex = nodes[nodeIndex + firstEdgeIndexOffset];
            var endEdgeIndex = nodeOrdinal < nodeCount - 1
                               ? nodes[nodeIndex + firstEdgeIndexOffset + nodeFieldCount]
                               : containmentEdges.length;
            for (var edgeIndex = beginEdgeIndex; edgeIndex < endEdgeIndex; edgeIndex += edgeFieldsCount) {
                var childNodeIndex = containmentEdges[edgeIndex + edgeToNodeOffset];
                if (flags[childNodeIndex / nodeFieldCount] & flag)
                    continue;
                var type = containmentEdges[edgeIndex + edgeTypeOffset];
                if (type === hiddenEdgeType || type === invisibleEdgeType || type === internalEdgeType)
                    continue;
                list.push(childNodeIndex);
            }
        }
    },

    _calculateFlags: function()
    {
        this._flags = new Uint32Array(this.nodeCount);
        this._markDetachedDOMTreeNodes();
        this._markQueriableHeapObjects();
    },

    calculateSnapshotDiff: function(baseSnapshotId, baseSnapshotAggregates)
    {
        var snapshotDiff = this._snapshotDiffs[baseSnapshotId];
        if (snapshotDiff)
            return snapshotDiff;
        snapshotDiff = {};

        var aggregates = this.aggregates(true, "allObjects");
        for (var className in baseSnapshotAggregates) {
            var baseAggregate = baseSnapshotAggregates[className];
            var diff = this._calculateDiffForClass(baseAggregate, aggregates[className]);
            if (diff)
                snapshotDiff[className] = diff;
        }
        var emptyBaseAggregate = { ids: [], indexes: [], selfSizes: [] };
        for (var className in aggregates) {
            if (className in baseSnapshotAggregates)
                continue;
            snapshotDiff[className] = this._calculateDiffForClass(emptyBaseAggregate, aggregates[className]);
        }

        this._snapshotDiffs[baseSnapshotId] = snapshotDiff;
        return snapshotDiff;
    },

    _calculateDiffForClass: function(baseAggregate, aggregate)
    {
        var baseIds = baseAggregate.ids;
        var baseIndexes = baseAggregate.indexes;
        var baseSelfSizes = baseAggregate.selfSizes;

        var indexes = aggregate ? aggregate.idxs : [];

        var i = 0, l = baseIds.length;
        var j = 0, m = indexes.length;
        var diff = { addedCount: 0,
                     removedCount: 0,
                     addedSize: 0,
                     removedSize: 0,
                     deletedIndexes: [],
                     addedIndexes: [] };

        var nodeB = new WebInspector.HeapSnapshotNode(this, indexes[j]);
        while (i < l && j < m) {
            var nodeAId = baseIds[i];
            if (nodeAId < nodeB.id) {
                diff.deletedIndexes.push(baseIndexes[i]);
                diff.removedCount++;
                diff.removedSize += baseSelfSizes[i];
                ++i;
            } else if (nodeAId > nodeB.id) { // Native nodes(e.g. dom groups) may have ids less than max JS object id in the base snapshot
                diff.addedIndexes.push(indexes[j]);
                diff.addedCount++;
                diff.addedSize += nodeB.selfSize;
                nodeB.nodeIndex = indexes[++j];
            } else { // nodeAId === nodeB.id
                ++i;
                nodeB.nodeIndex = indexes[++j];
            }
        }
        while (i < l) {
            diff.deletedIndexes.push(baseIndexes[i]);
            diff.removedCount++;
            diff.removedSize += baseSelfSizes[i];
            ++i;
        }
        while (j < m) {
            diff.addedIndexes.push(indexes[j]);
            diff.addedCount++;
            diff.addedSize += nodeB.selfSize;
            nodeB.nodeIndex = indexes[++j];
        }
        diff.countDelta = diff.addedCount - diff.removedCount;
        diff.sizeDelta = diff.addedSize - diff.removedSize;
        if (!diff.addedCount && !diff.removedCount)
            return null;
        return diff;
    },

    _nodeForSnapshotObjectId: function(snapshotObjectId)
    {
        for (var it = this._allNodes; it.hasNext(); it.next()) {
            if (it.node.id === snapshotObjectId)
                return it.node;
        }
        return null;
    },

    nodeClassName: function(snapshotObjectId)
    {
        var node = this._nodeForSnapshotObjectId(snapshotObjectId);
        if (node)
            return node.className;
        return null;
    },

    dominatorIdsForNode: function(snapshotObjectId)
    {
        var node = this._nodeForSnapshotObjectId(snapshotObjectId);
        if (!node)
            return null;
        var result = [];
        while (!node.isRoot) {
            result.push(node.id);
            node.nodeIndex = node.dominatorIndex;
        }
        return result;
    },

    _parseFilter: function(filter)
    {
        if (!filter)
            return null;
        var parsedFilter = eval("(function(){return " + filter + "})()");
        return parsedFilter.bind(this);
    },

    createEdgesProvider: function(nodeIndex, filter)
    {
        var node = new WebInspector.HeapSnapshotNode(this, nodeIndex);
        return new WebInspector.HeapSnapshotEdgesProvider(this, this._parseFilter(filter), node.edges);
    },

    createRetainingEdgesProvider: function(nodeIndex, filter)
    {
        var node = new WebInspector.HeapSnapshotNode(this, nodeIndex);
        return new WebInspector.HeapSnapshotEdgesProvider(this, this._parseFilter(filter), node.retainers);
    },

    createAddedNodesProvider: function(baseSnapshotId, className)
    {
        var snapshotDiff = this._snapshotDiffs[baseSnapshotId];
        var diffForClass = snapshotDiff[className];
        return new WebInspector.HeapSnapshotNodesProvider(this, null, diffForClass.addedIndexes);
    },

    createDeletedNodesProvider: function(nodeIndexes)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, null, nodeIndexes);
    },

    createNodesProviderForClass: function(className, aggregatesKey)
    {
        return new WebInspector.HeapSnapshotNodesProvider(this, null, this.aggregates(false, aggregatesKey)[className].idxs);
    },

    createNodesProviderForDominator: function(nodeIndex)
    {
        var node = new WebInspector.HeapSnapshotNode(this, nodeIndex);
        return new WebInspector.HeapSnapshotNodesProvider(this, null, this._dominatedNodesOfNode(node));
    },

    updateStaticData: function()
    {
        return {nodeCount: this.nodeCount, rootNodeIndex: this._rootNodeIndex, totalSize: this.totalSize, uid: this.uid, nodeFlags: this._nodeFlags};
    }
};

/**
 * @constructor
 * @param {Array.<number>=} unfilteredIterationOrder
 */
WebInspector.HeapSnapshotFilteredOrderedIterator = function(iterator, filter, unfilteredIterationOrder)
{
    this._filter = filter;
    this._iterator = iterator;
    this._unfilteredIterationOrder = unfilteredIterationOrder;
    this._iterationOrder = null;
    this._position = 0;
    this._currentComparator = null;
    this._sortedPrefixLength = 0;
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype = {
    _createIterationOrder: function()
    {
        if (this._iterationOrder)
            return;
        if (this._unfilteredIterationOrder && !this._filter) {
            this._iterationOrder = this._unfilteredIterationOrder.slice(0);
            this._unfilteredIterationOrder = null;
            return;
        }
        this._iterationOrder = [];
        var iterator = this._iterator;
        if (!this._unfilteredIterationOrder && !this._filter) {
            for (iterator.first(); iterator.hasNext(); iterator.next())
                this._iterationOrder.push(iterator.index);
        } else if (!this._unfilteredIterationOrder) {
            for (iterator.first(); iterator.hasNext(); iterator.next()) {
                if (this._filter(iterator.item))
                    this._iterationOrder.push(iterator.index);
            }
        } else {
            var order = this._unfilteredIterationOrder.constructor === Array ?
                this._unfilteredIterationOrder : this._unfilteredIterationOrder.slice(0);
            for (var i = 0, l = order.length; i < l; ++i) {
                iterator.index = order[i];
                if (this._filter(iterator.item))
                    this._iterationOrder.push(iterator.index);
            }
            this._unfilteredIterationOrder = null;
        }
    },

    first: function()
    {
        this._position = 0;
    },

    hasNext: function()
    {
        return this._position < this._iterationOrder.length;
    },

    get isEmpty()
    {
        if (this._iterationOrder)
            return !this._iterationOrder.length;
        if (this._unfilteredIterationOrder && !this._filter)
            return !this._unfilteredIterationOrder.length;
        var iterator = this._iterator;
        if (!this._unfilteredIterationOrder && !this._filter) {
            iterator.first();
            return !iterator.hasNext();
        } else if (!this._unfilteredIterationOrder) {
            for (iterator.first(); iterator.hasNext(); iterator.next())
                if (this._filter(iterator.item))
                    return false;
        } else {
            var order = this._unfilteredIterationOrder.constructor === Array ?
                this._unfilteredIterationOrder : this._unfilteredIterationOrder.slice(0);
            for (var i = 0, l = order.length; i < l; ++i) {
                iterator.index = order[i];
                if (this._filter(iterator.item))
                    return false;
            }
        }
        return true;
    },

    get item()
    {
        this._iterator.index = this._iterationOrder[this._position];
        return this._iterator.item;
    },

    get length()
    {
        this._createIterationOrder();
        return this._iterationOrder.length;
    },

    next: function()
    {
        ++this._position;
    },

    /**
     * @param {number} begin
     * @param {number} end
     */
    serializeItemsRange: function(begin, end)
    {
        this._createIterationOrder();
        if (begin > end)
            throw new Error("Start position > end position: " + begin + " > " + end);
        if (end >= this._iterationOrder.length)
            end = this._iterationOrder.length;
        if (this._sortedPrefixLength < end) {
            this.sort(this._currentComparator, this._sortedPrefixLength, this._iterationOrder.length - 1, end - this._sortedPrefixLength);
            this._sortedPrefixLength = end;
        }

        this._position = begin;
        var startPosition = this._position;
        var count = end - begin;
        var result = new Array(count);
        for (var i = 0 ; i < count && this.hasNext(); ++i, this.next())
            result[i] = this.serializeItem(this.item);
        result.length = i;
        result.totalLength = this._iterationOrder.length;

        result.startPosition = startPosition;
        result.endPosition = this._position;
        return result;
    },

    sortAll: function()
    {
        this._createIterationOrder();
        if (this._sortedPrefixLength === this._iterationOrder.length)
            return;
        this.sort(this._currentComparator, this._sortedPrefixLength, this._iterationOrder.length - 1, this._iterationOrder.length);
        this._sortedPrefixLength = this._iterationOrder.length;
    },

    sortAndRewind: function(comparator)
    {
        this._currentComparator = comparator;
        this._sortedPrefixLength = 0;
        this.first();
    }
}

WebInspector.HeapSnapshotFilteredOrderedIterator.prototype.createComparator = function(fieldNames)
{
    return {fieldName1:fieldNames[0], ascending1:fieldNames[1], fieldName2:fieldNames[2], ascending2:fieldNames[3]};
}

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotFilteredOrderedIterator}
 */
WebInspector.HeapSnapshotEdgesProvider = function(snapshot, filter, edgesIter)
{
    this.snapshot = snapshot;
    WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, edgesIter, filter);
}

WebInspector.HeapSnapshotEdgesProvider.prototype = {
    serializeItem: function(edge)
    {
        return {
            name: edge.name,
            propertyAccessor: edge.toString(),
            node: WebInspector.HeapSnapshotNodesProvider.prototype.serializeItem(edge.node),
            nodeIndex: edge.nodeIndex,
            type: edge.type,
            distanceToWindow: edge.node.distanceToWindow
        };
    },

    sort: function(comparator, leftBound, rightBound, count)
    {
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var edgeA = this._iterator.item.clone();
        var edgeB = edgeA.clone();
        var nodeA = new WebInspector.HeapSnapshotNode(this.snapshot);
        var nodeB = new WebInspector.HeapSnapshotNode(this.snapshot);

        function compareEdgeFieldName(ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            edgeB.edgeIndex = indexB;
            if (edgeB.name === "__proto__") return -1;
            if (edgeA.name === "__proto__") return 1;
            var result =
                edgeA.hasStringName === edgeB.hasStringName ?
                (edgeA.name < edgeB.name ? -1 : (edgeA.name > edgeB.name ? 1 : 0)) :
                (edgeA.hasStringName ? -1 : 1);
            return ascending ? result : -result;
        }

        function compareNodeField(fieldName, ascending, indexA, indexB)
        {
            edgeA.edgeIndex = indexA;
            nodeA.nodeIndex = edgeA.nodeIndex;
            var valueA = nodeA[fieldName];

            edgeB.edgeIndex = indexB;
            nodeB.nodeIndex = edgeB.nodeIndex;
            var valueB = nodeB[fieldName];

            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        function compareEdgeAndNode(indexA, indexB) {
            var result = compareEdgeFieldName(ascending1, indexA, indexB);
            if (result === 0)
                result = compareNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        function compareNodeAndEdge(indexA, indexB) {
            var result = compareNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = compareEdgeFieldName(ascending2, indexA, indexB);
            return result;
        }

        function compareNodeAndNode(indexA, indexB) {
            var result = compareNodeField(fieldName1, ascending1, indexA, indexB);
            if (result === 0)
                result = compareNodeField(fieldName2, ascending2, indexA, indexB);
            return result;
        }

        if (fieldName1 === "!edgeName")
            this._iterationOrder.sortRange(compareEdgeAndNode, leftBound, rightBound, count);
        else if (fieldName2 === "!edgeName")
            this._iterationOrder.sortRange(compareNodeAndEdge, leftBound, rightBound, count);
        else
            this._iterationOrder.sortRange(compareNodeAndNode, leftBound, rightBound, count);
    }
};

WebInspector.HeapSnapshotEdgesProvider.prototype.__proto__ = WebInspector.HeapSnapshotFilteredOrderedIterator.prototype;

/**
 * @constructor
 * @extends {WebInspector.HeapSnapshotFilteredOrderedIterator}
 * @param {Array.<number>=} nodeIndexes
 */
WebInspector.HeapSnapshotNodesProvider = function(snapshot, filter, nodeIndexes)
{
    this.snapshot = snapshot;
    WebInspector.HeapSnapshotFilteredOrderedIterator.call(this, snapshot._allNodes, filter, nodeIndexes);
}

WebInspector.HeapSnapshotNodesProvider.prototype = {
    nodePosition: function(snapshotObjectId)
    {
        this._createIterationOrder();
        if (this.isEmpty)
            return -1;
        this.sortAll();

        var node = new WebInspector.HeapSnapshotNode(this.snapshot);
        for (var i = 0; i < this._iterationOrder.length; i++) {
            node.nodeIndex = this._iterationOrder[i];
            if (node.id === snapshotObjectId)
                return i;
        }
        return -1;
    },

    serializeItem: function(node)
    {
        return {
            id: node.id,
            name: node.name,
            distanceToWindow: node.distanceToWindow,
            nodeIndex: node.nodeIndex,
            retainedSize: node.retainedSize,
            selfSize: node.selfSize,
            type: node.type,
            flags: node.flags
        };
    },

    sort: function(comparator, leftBound, rightBound, count)
    {
        var fieldName1 = comparator.fieldName1;
        var fieldName2 = comparator.fieldName2;
        var ascending1 = comparator.ascending1;
        var ascending2 = comparator.ascending2;

        var nodeA = new WebInspector.HeapSnapshotNode(this.snapshot);
        var nodeB = new WebInspector.HeapSnapshotNode(this.snapshot);

        function sortByNodeField(fieldName, ascending)
        {
            var valueA = nodeA[fieldName];
            var valueB = nodeB[fieldName];
            var result = valueA < valueB ? -1 : (valueA > valueB ? 1 : 0);
            return ascending ? result : -result;
        }

        function sortByComparator(indexA, indexB) {
            nodeA.nodeIndex = indexA;
            nodeB.nodeIndex = indexB;
            var result = sortByNodeField(fieldName1, ascending1);
            if (result === 0)
                result = sortByNodeField(fieldName2, ascending2);
            return result;
        }

        this._iterationOrder.sortRange(sortByComparator, leftBound, rightBound, count);
    }
};

WebInspector.HeapSnapshotNodesProvider.prototype.__proto__ = WebInspector.HeapSnapshotFilteredOrderedIterator.prototype;
