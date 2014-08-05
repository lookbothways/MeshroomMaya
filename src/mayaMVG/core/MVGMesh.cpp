#include "mayaMVG/core/MVGMesh.h"
#include "mayaMVG/core/MVGLog.h"
#include "mayaMVG/core/MVGGeometryUtil.h"
#include "mayaMVG/core/MVGProject.h"
#include "mayaMVG/maya/MVGMayaUtil.h"
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MSelectionList.h>
#include <maya/MPointArray.h>
#include <maya/MIntArray.h>
#include <maya/MDagModifier.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>

using namespace mayaMVG;

MVGMesh::MVGMesh(const std::string& name)
	: MVGNodeWrapper(name)
{
}

MVGMesh::MVGMesh(const MDagPath& dagPath)
	: MVGNodeWrapper(dagPath)
{
}

MVGMesh::~MVGMesh()
{
}

// virtual
bool MVGMesh::isValid() const
{
	return _dagpath.isValid();
}

// static
MVGMesh MVGMesh::create(const std::string& name)
{
	MStatus status;
	MFnMesh fnMesh;

	// get project root node
	MVGProject project(MVGProject::_PROJECT);
	MObject rootObj = project.dagPath().child(2); // meshes transform

	// create empty mesh
	MPointArray vertexArray;
	MIntArray polygonCounts, polygonConnects;
	MObject transform = fnMesh.create(0, 0, vertexArray, polygonCounts, 
					polygonConnects, MObject::kNullObj, &status);
	
	// register dag path
	MDagPath path;
	MDagPath::getAPathTo(transform, path);
	path.extendToShape();

	// connect to initialShadingGroup
	MObject sgObj;
	MSelectionList list;
	status = MGlobal::getSelectionListByName("initialShadingGroup", list);
	status = list.getDependNode(0, sgObj);
	MFnSet fnSet(sgObj, &status);
 	status = fnSet.addMember(path);
	CHECK(status)

	// reparent under root node
	MDagModifier dagModifier;
	dagModifier.reparentNode(transform, rootObj);
	dagModifier.doIt();

	// rename and return
	MVGMesh mesh(path);
	mesh.setName(name);
	return mesh;
}

bool MVGMesh::addPolygon(const MPointArray& pointArray, int& index) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	if(pointArray.length() < 3)
		return false;
	
	fnMesh.addPolygon(pointArray, index, true, 0.01, MObject::kNullObj, &status);
	fnMesh.updateSurface();
	CHECK(status)
	return status ? true : false;
}

bool MVGMesh::deletePolygon(const int index) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	status = fnMesh.deleteFace(index);
	fnMesh.updateSurface();
	CHECK(status);
	return status ? true : false;
}

void MVGMesh::getPoints(MPointArray& pointArray) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	fnMesh.getPoints(pointArray, MSpace::kWorld);
	CHECK(status);
}

int MVGMesh::getVerticesCount() const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	int count = fnMesh.numVertices(&status);
	CHECK(status);
	return count;
}

int MVGMesh::getPolygonsCount() const
{
    MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
    int count = fnMesh.numPolygons(&status);
    CHECK(status);
    return count;
}

bool MVGMesh::intersect(MPoint& point, MVector& dir, MPointArray&points) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	bool intersect = fnMesh.intersect(point, dir, points, status);
	CHECK(status);
	return intersect;
}

int MVGMesh::getNumConnectedFacesToVertex(int vertexId) const
{
	MStatus status;
	MItMeshVertex verticesIter(_dagpath, MObject::kNullObj, &status);
	CHECK(status);
	int prev;
	status = verticesIter.setIndex(vertexId, prev);
	int faceCount;
	status = verticesIter.numConnectedFaces(faceCount);
	CHECK(status);
	return faceCount;
}

int MVGMesh::getNumConnectedFacesToEdge(int edgeId) const
{
	MStatus status;
	MItMeshEdge edgeIter(_dagpath, MObject::kNullObj, &status);
	CHECK(status);
	int prev;
	status = edgeIter.setIndex(edgeId, prev);
	int faceCount;
	status = edgeIter.numConnectedFaces(faceCount);
	CHECK(status);
	return faceCount;
}

const MIntArray MVGMesh::getConnectedFacesToVertex(int vertexId) const
{
	MIntArray connectedFacesId;
	MStatus status;
	MItMeshVertex verticesIter(_dagpath, MObject::kNullObj, &status);
	CHECK(status);
	int prev;
	status = verticesIter.setIndex(vertexId, prev);
	CHECK(status);
	status = verticesIter.getConnectedFaces(connectedFacesId);
	CHECK(status);
	return connectedFacesId;
}

int MVGMesh::getConnectedFacesToEdge(MIntArray& facesId, int edgeId) const
{
	MStatus status;
	MItMeshEdge edgeIter(_dagpath, MObject::kNullObj, &status);
	CHECK(status);
	int prev;
	status = edgeIter.setIndex(edgeId, prev);
	CHECK(status);
	int faceCount;
	faceCount = edgeIter.getConnectedFaces(facesId, &status);
	CHECK(status);
	return faceCount;
}


const MIntArray MVGMesh::getFaceVertices(int faceId) const
{
	MStatus status;
	MItMeshPolygon faceIter(_dagpath);
	int prev;
	status = faceIter.setIndex(faceId, prev);
	CHECK(status);
	MIntArray vertices;
	status = faceIter.getVertices(vertices);
	CHECK(status);
	return vertices;
}

const MIntArray MVGMesh::getEdgeVertices(int edgeId) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	int2 edgeVertices;
	status = fnMesh.getEdgeVertices(edgeId, edgeVertices);
	CHECK(status);
	MIntArray edgeVerticesArray;
	edgeVerticesArray.append(edgeVertices[0]);
	edgeVerticesArray.append(edgeVertices[1]);
	return edgeVerticesArray;
}

MStatus MVGMesh::setPoint(int vertexId, MPoint& point) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	CHECK(status);
	status = fnMesh.setPoint(vertexId, point, MSpace::kWorld);
	fnMesh.updateSurface();
	CHECK(status);
	return status;
}

MStatus MVGMesh::getPoint(int vertexId, MPoint& point) const
{
	MStatus status;
	MFnMesh fnMesh(_dagpath, &status);
	status = fnMesh.getPoint(vertexId, point, MSpace::kWorld);
	CHECK(status);
	return status;
}
