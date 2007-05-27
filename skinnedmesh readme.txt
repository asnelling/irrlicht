==========================================================================
Converting bone based mesh loaders to work with one internal format
==========================================================================

  Table of contents

  1. Status/ToDo
  2. Using skinned meshes
  3. Hardware skinning
  4. Writing a new loader

==========================================================================
1. Status/ToDo
==========================================================================

Irrlicht suffers from having different skinning methods for each mesh
format, because the mesh formats and animation are controlled by the
loaders.

This project is currently in its infancy, before we can merge it into
Irrlicht we should do the following things-

  1. Finish CSkinnedMesh, for animation playback.
     User bone control via IBoneSceneNode.

  2. Separate mesh loaders from IAnimatedMesh convert to proper loaders
     derived from IMeshLoader, and add options for them to be compiled
     out of Irrlicht via irrCompileConfig.h.
     Only one of the loaders needs to be complete before we merge-

     a. CAnimatedMeshB3d becomes CB3DMeshLoader.

     b. CAnimatedMeshMS3D becomes CMS3DMeshLoader, looks simple.

     c. For the moment keep CXMeshFileReader, and move code from
        CXAnimationPlayer::createAnimationData to CXMeshFileLoader.
        Currently X files can't have nested joints, which isn't a show-
        stopper but isn't very nice.
        This really needs to be fixed, but possibly not for this project.

  3. Adapt CAnimatedMeshSceneNode to work with CSkinnedMesh, remove old
     bone access methods and replace with new ones.


Detailed check list:

CSkinnedMesh:

[X] Add an interface for the loaders to create this mesh (may need more work)

[ ] Get/store the keyframe indexes from bones to avoid searches

CB3DMeshLoader:

[X] covert it to create an CSkinnedMesh using it's interface.

[X] Fully load a mesh and get skinnedMesh to animate it

CAnimatedMeshSceneNode:

[ ] fix bug to double the speed of irrlicht's animations

[ ] get this to use SkinnedMesh's features, for bone control, animation blending, transitions...

[ ] Change it from calling GetMesh() when holding SkinnedMesh, to the newer functions, this will also stop the need for the loaders scaling the animations out for interpolation, and will make animations a lot smoother.



==========================================================================
2. Using skinned mesh scene nodes
==========================================================================

todo: Section to explain how to use the new AnimatedMeshSceneNode from a
user perspective.

==========================================================================
3. Hardware skinning
==========================================================================

Should be possible in the future by embedding the skin weights in the
vertex data, for example in the 2nd set of texture coordinates.

How to do this is open for debate.
Do we need/should we have a new vertex format?

==========================================================================
4. Writing a new loader
==========================================================================

