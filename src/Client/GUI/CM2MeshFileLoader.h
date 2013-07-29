#include "irrlicht/irrlicht.h"
#include "irrlicht/IMeshLoader.h"
#include "CM2Mesh.h"
#include <string>
#include <vector>
#include <algorithm>

namespace irr
{
namespace scene
{

enum ABlockDatatype{
  ABDT_FLOAT,
  ABDT_SHORT,
  ABDT_INT
};

struct numofs {
    u32 num;
    u32 ofs;
};


struct ModelHeader {
    c8 id[4];               //0x00
    u32 version;
    numofs name;
    u32 type;               //0x10

    //Anim Block @ 0x14
    numofs GlobalSequences;
    numofs Animations;
    numofs AnimationLookup;
    numofs D;                   //Absent in WOTLK
    numofs Bones;
    numofs SkelBoneLookup;
    numofs Vertices;
    numofs Views;               //ofs Absent in WOTLK
    numofs Colors;
    numofs Textures;
    numofs Transparency;
    numofs I;                   //Absent in WoTLK
    numofs TexAnims;
    numofs TexReplace;
    numofs TexFlags;
    numofs BoneLookupTable;
    numofs TexLookup;
    numofs TexUnitLookup;
    numofs TransparencyLookup;
    numofs TexAnimLookup;

	//f32 floats[14];
	core::vector3df VertexBox1; 
	core::vector3df VertexBox2;
	float VertexRadius;
	core::vector3df BoundingBox1; 
	core::vector3df BoundingBox2;
	float BoundingRadius;

    numofs BoundingTriangles;
    numofs BoundingVertices;
    numofs BoundingNormals;

    numofs Attachments;
    numofs AttachLookup;
    numofs Events;
    numofs Lights;
    numofs Cameras;
    numofs CameraLookup;
    numofs RibbonEmitters;
    numofs ParticleEmitters;

	//WOTLK has one more field which is only set under specific conditions.

};

struct TextureDefinition {
    u32 texType;
    u16 unk;
    u16 texFlags;
    u32 texFileLen;
    u32 texFileOfs;
};

struct ModelVertex {
	core::vector3df pos;
	u8 weights[4];
	u8 bones[4];
	core::vector3df normal;
	core::vector2df texcoords;
	u32 unk1, unk2; // always 0,0 so this is probably unused
};

struct ModelView {
    numofs Index;       // Vertices in this model (index into vertices[])
    numofs Triangle;    // indices
    numofs Properties;  // additional vtx properties (mostly 0?)
    numofs Submesh;     // submeshes
    numofs Tex;         // material properties/textures
    u32 lod;            // LOD bias? unknown
};

struct ModelViewSubmesh { //Curse you BLIZZ for not using numofs here
    u32 meshpartId;
    u16 ofsVertex;//Starting vertex number for this submesh
    u16 nVertex;
    u16 ofsTris;//Starting Triangle index
    u16 nTris;
    u16 nBone, ofsBone, unk3, unk4;
    core::vector3df CenterOfMass; // Average of all vertices
	core::vector3df BB; // center of an axis aligned bounding box built around all this submesh's vertices
    float Radius; // Submesh Radius
};

struct TextureUnit{
    u16 Flags;         // 2 u8 first is texture animation 0=true, second flag is some sort of submesh grouping
    s16 renderOrder;   // 2 more u8 indicating what shaders to use
    u16 submeshIndex1, submeshIndex2;
    s16 colorIndex;
    u16 renderFlagsIndex;
    u16 TextureUnitNumber;
    u16 Mode;
    u16 textureIndex;
    u16 TextureUnitNumber2;
    u16 transparencyIndex;
    u16 texAnimIndex;
};

struct RenderFlags{
    u16 flags;
    u16 blending;
};

struct RawAnimation{
    u32 animationID;
    u32 start, end;
    float movespeed;
    u32 loop, probability, unk1, unk2;
    u32 playbackspeed;
    float bbox[6];
    float radius;
    s16 indexSameID;
    u16 unk3;
};

struct RawAnimationWOTLK{
    u16 animationID, subanimationID;
    u32 length;
    float movespeed;
    u32 flags;
    u16 probability, unused;
    u32 unk1, unk2, playbackspeed;
    float bbox[6];
    float radius;
    s16 indexSameID;
    u16 index;
};

struct Animation{
  u32 animationID;
  u32 subanimationID;
  u32 start, end;
  u32 flags;
  f32 probability;
};

struct AnimBlockHead{
    s16 interpolationType;
    s16 globalSequenceID;
    numofs InterpolationRanges;    //Missing in WotLK
    numofs TimeStamp;
    numofs Values;
};

// struct InterpolationRange{
//     u32 start, end;
// };

struct AnimBlock{
    AnimBlockHead header;
//     core::array<InterpolationRange> keyframes;  // We are not using this
    core::array<u32> timestamps;
    core::array<float> values;
};

struct Bone{
    s32 SkelBoneIndex;
    u32 flags;
    s16 parentBone;
    u16 unk1;
    u16 unk2;
    u16 unk3;
    AnimBlock translation, rotation, scaling;
    core::vector3df PivotPoint;
};

struct VertexColor{
    AnimBlock Colors;
    AnimBlock Alpha;
};


struct Light{
    u16 Type;
    s16 Bone;
    core::vector3df Position;
    AnimBlock AmbientColor;
    AnimBlock AmbientIntensity;
    AnimBlock DiffuseColor;
    AnimBlock DiffuseIntensity;
    AnimBlock AttenuationStart;
    AnimBlock AttenuationEnd;
    AnimBlock Unknown;
};


// Structure to contain all arrays from a .skin file
struct SkinData{
	core::array<u16> M2MIndices;
	core::array<u16> M2MTriangles;
	core::array<ModelViewSubmesh> M2MSubmeshes;
	core::array<TextureUnit> M2MTextureUnit;
};


class CM2MeshFileLoader : public IMeshLoader
{
public:

	//! Constructor
	CM2MeshFileLoader(IrrlichtDevice* device);

	//! destructor
	virtual ~CM2MeshFileLoader();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".cob")
	virtual bool isALoadableFileExtension(const io::path& fileName)const;

	//! creates/loads an animated mesh from the file.
	//! \return Pointer to the created mesh. Returns 0 if loading failed.
	//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
	//! See IUnknown::drop() for more information.
	virtual scene::IAnimatedMesh* createMesh(io::IReadFile* file);
private:

	bool load();
    void ReadBones();
	void ReadColors();
	void ReadLights();
    void ReadVertices();
    void ReadTextureDefinitions();
    void ReadAnimationData();
    void ReadViewData(io::IReadFile* file);
    void ReadABlock(AnimBlock &ABlock, u8 datatype, u8 datanum);
	void CopyAnimationsToMesh(CM2Mesh * CurrentMesh);
	void BuildANewSubMesh(CM2Mesh * CurrentMesh, u32 v, u32 i); // v is index to current veiw and i is the index for the current submesh in this view 

	IrrlichtDevice *Device;
    core::stringc Texdir;
    io::IReadFile *MeshFile, *SkinFile;

    CM2Mesh *AnimatedMesh;
    scene::CM2Mesh::SJoint *ParentJoint;



    ModelHeader header;
    ModelView currentView;
    core::stringc M2MeshName;
    SMesh* Mesh;
    //SSkinMeshBuffer* MeshBuffer;
    //Taken from the Model file, thus m2M*
	core::array<Light> M2MLights;
	core::array<VertexColor> M2MVertexColor;
    core::array<ModelVertex> M2MVertices;
    core::array<u16> M2MIndices;
    core::array<u16> M2MTriangles;
    core::array<ModelViewSubmesh> M2MSubmeshes;
    core::array<u16> M2MTextureLookup;
    core::array<TextureDefinition> M2MTextureDef;
    core::array<std::string> M2MTextureFiles;
    core::array<TextureUnit> M2MTextureUnit;
    core::array<RenderFlags> M2MRenderFlags;
	core::array<SkinData> M2MSkins; // each element is a view or skin and contains all arrays from a .skin file
    core::array<u32> M2MGlobalSequences;
    core::array<Animation> M2MAnimations;
    core::array<io::IReadFile*> M2MAnimfiles;//Stores pointers to *.anim files in WOTLK

    core::array<s16> M2MAnimationLookup;
    core::array<Bone> M2MBones;
    core::array<u16> M2MBoneLookupTable;
    core::array<u16> M2MSkeleBoneLookupTable;
    //Used for the Mesh, thus m2_noM_*
    core::array<video::S3DVertex> M2Vertices;
    core::array<u16> M2Indices;
    core::array<scene::ISkinnedMesh::SJoint> M2Joints;


	void sortDistance(core::array<CM2Mesh::submesh> &Submesh)
	{
		CM2Mesh::submesh temp;
		for (int i = 0; i < Submesh.size()-1; i++) 
		{
			for (u16 j = i+1; j < Submesh.size(); j++)
			{
				if (Submesh[i].Distance < Submesh[j].Distance)
				{
					temp = Submesh[i];
					Submesh[i] = Submesh[j];
					Submesh[j] = temp;
				}
			}
		}
	}

	void dropDecalsToTheirBackdrops (core::array<CM2Mesh::submesh> &Submeshes, u16 skin)
	{
		for (int i=0; i<Submeshes.size(); i++)
		{
			 if (Submeshes[i].Textures[0].shaderType == 2) // if decal
			 {
				  s16 index = i-1; // index to previous element
				  while (index >= 0)
				  {
					   bool move = false;  // if this becomes true decal will be moved to index+1
					   if (Submeshes[index].Textures[0].BlendFlag < 2) // skip/ignore other effects
					   {
						   float radius = M2MSkins[skin].M2MSubmeshes[index].Radius;  // AnimatedMesh->SkinID
							//  Only if decal's center of mass is between the other element's top and bottom test for other overlaps
							if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y+radius)
							{ 
									// test x overlap
									if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.X > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.X-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.X < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.X+radius)
									{
										move = true;
									}
									// test z overlap;
									if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Z > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Z-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Z < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Z+radius)
									{
										move = true;
								 }
							}
					   }
					   if (move == true)
					   {
							CM2Mesh::submesh temp = Submeshes[i];
							Submeshes.erase(i);
							Submeshes.insert(temp, index+1);
							index = -1; // the decal was put in place so the loop can end now so we can find the next decal
					   }
					   index--; // move the index back an element
				  }
			 }
		}
	}

	void sortSizeBracketByMode (core::array<CM2Mesh::submesh> &Submesh) // b=bracket start, B=bracket end, s=skin index
	{
		CM2Mesh::submesh temp;
		//CM2Mesh::BufferInfo temp;
		for (int i = 0; i < Submesh.size()-1; i++) 
		{
			for (u16 j = i+1; j < Submesh.size(); j++)
			{/*
				if (AnimatedMesh->BufferMap[i].Mode > AnimatedMesh->BufferMap[j].Mode)
				{
					temp = AnimatedMesh->BufferMap[i];
					AnimatedMesh->BufferMap[i] = AnimatedMesh->BufferMap[j];
					AnimatedMesh->BufferMap[j] = temp;
				}*/
				if (Submesh[i].Textures[0].Mode > Submesh[j].Textures[0].Mode)  // AnimatedMesh->Skins[s].Submeshes
				{
					temp = Submesh[i];
					Submesh[i] = Submesh[j];
					Submesh[j] = temp;
				}

			}
		}
	}


	void sortModeByBlock(core::array<CM2Mesh::submesh> &Submesh) // b=bracket start, B=bracket end, s=skin index
	{
		CM2Mesh::submesh temp;
		//CM2Mesh::BufferInfo temp;
		for (int i = 0; i <= Submesh.size()-1; i++) 
		{
			for (u16 j = i+1; j < Submesh.size(); j++)
			{
				// if they are the same mode sort by block
				/*if (AnimatedMesh->BufferMap[i].Mode == AnimatedMesh->BufferMap[j].Mode && AnimatedMesh->BufferMap[i].block > AnimatedMesh->BufferMap[j].block)
				{
					temp = AnimatedMesh->BufferMap[i];
					AnimatedMesh->BufferMap[i] = AnimatedMesh->BufferMap[j];
					AnimatedMesh->BufferMap[j] = temp;
				}*/
				if (Submesh[i].Textures[0].Mode == Submesh[j].Textures[0].Mode && Submesh[i].Textures[0].Block > Submesh[j].Textures[0].Block)
				{
					temp = Submesh[i];
					Submesh[i] = Submesh[j];
					Submesh[j] = temp;
				}
			}
		}
	}


	void sortRadius(int b, int B, int s) // b=bracket start, B=bracket end, s=skin index
	{
		//CM2Mesh::submesh temp;
		CM2Mesh::BufferInfo temp;
		for (int i = b; i <= B; i++) 
		{
			for (u16 j = b+1; j <= B; j++)
			{
				if (AnimatedMesh->BufferMap[i].Radius > AnimatedMesh->BufferMap[j].Radius)
				{
					temp = AnimatedMesh->BufferMap[i];
					AnimatedMesh->BufferMap[i] = AnimatedMesh->BufferMap[j];
					AnimatedMesh->BufferMap[j] = temp;
				}
				/*if (AnimatedMesh->Skins[s].Submeshes[i].Radius > AnimatedMesh->Skins[s].Submeshes[j].Radius)
				{
					temp = AnimatedMesh->Skins[s].Submeshes[i];
					AnimatedMesh->Skins[s].Submeshes[i] = AnimatedMesh->Skins[s].Submeshes[j];
					AnimatedMesh->Skins[s].Submeshes[j] = temp;
				}*/
			}
		}
	}


	void sortPointHighLow (core::array<scene::CM2Mesh::BufferInfo> &BlockList)
	{
		  scene::CM2Mesh::BufferInfo temp;

		  for(int i = 0; i < BlockList.size( ) - 1; i++)
		  {
			   for (int j = i + 1; j < BlockList.size( ); j++)
			   {
				   if (BlockList[ i ].Back == BlockList[ j ].Back && BlockList[ i ].Front == BlockList[ j ].Front && BlockList[ i ].block > BlockList[ j ].block) // If 2 submeshes are in exactly the same spot the one with a higher block value is nearest
				   {
					   temp = BlockList[ i ];    //swapping entire element
					   BlockList[ i ] = BlockList[ j ];
					   BlockList[ j ] = temp;
				   }
				   else if (BlockList[ i ].SortPoint < BlockList[ j ].SortPoint) // The highest sort point value is farthest from camera
				   {
					   temp = BlockList[ i ];    //swapping entire element
					   BlockList[ i ] = BlockList[ j ];
					   BlockList[ j ] = temp;
				   }
			   }
		  }
		  return;
	}
	
	void InsertOverlays (core::array<scene::CM2Mesh::BufferInfo> &OverLay, core::array<scene::CM2Mesh::BufferInfo> &Destination)
	{
		int Position = -1;     // The starting position of the overlay
		// Loop through overlays
		for(int O = 0; O < OverLay.size( ) - 1; O++)
		{ 
			// Now find the position the overlay should have in the destination array
			//for(int P = 0; P < Destination.size() - 1; P++)
			int P = 0;
			while (Position == -1)
			{
				if(OverLay[O].SortPoint > Destination[P].SortPoint)   // if we found the first mesh physicaly nearer than the overlay
				{
					Position = P-1;                                   // Set starting position index for the overly in the destination array
					//P=Destination.size() - 1;                         // end this loop since we found this overlay's position
				}
				P++;
			}
			int d = Position;  // The starting point to work back from
			// Loop backwards through the Destination array starting at element [Position] and ending at the qualifing element
			bool loop = true;
			while(loop == true)  //for(int d = 0; d <Destination.size( ) - 1; d++)  //for(d > (-1); d--)
			{
				// Move backwards from Position compairing each element to the current overlay to find the first element 
				// larger than the overlay and directly behind it (not offset to the side)
				// LeftHand cords +x are to the right and +y are up.  Should I get the real width and height insted of subing radius? 
				//if(OverLay[O].Coordinates.X+OverLay[O].Radius <= Destination[d].Coordinates.X+Destination[d].Radius && OverLay[O].Coordinates.X-OverLay[O].Radius <= Destination[d].Coordinates.X-Destination[d].Radius && OverLay[O].Coordinates.Y+OverLay[O].Radius <= Destination[d].Coordinates.Y+Destination[d].Radius && OverLay[O].Coordinates.Y-OverLay[O].Radius <= Destination[d].Coordinates.Y-Destination[d].Radius)
				if(OverLay[O].Xpos <= Destination[d].Xpos && OverLay[O].Xneg >= Destination[d].Xneg && OverLay[O].Ypos <= Destination[d].Ypos && OverLay[O].Yneg >= Destination[d].Yneg)
				{
					// Insert the overlay on top of the qualified element
					Destination.insert(OverLay[O], d+1);
					// End loop so we don't insert multiple copies
					loop = false;
				}
				d--;  // Move to the next farthest element
			}
		}
		return;
	}

};
}//namespace scene
}//namespace irr
