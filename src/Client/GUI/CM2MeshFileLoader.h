#include <iostream>
#include "MemoryDataHolder.h"
#include "MemoryInterface.h"
#include "irrlicht/irrlicht.h"
#include "irrlicht/IMeshLoader.h"
#include "CM2Mesh.h"
#include <string>
#include <vector>
#include <algorithm>
#include "common.h"

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

struct UVAnimation{
	AnimBlock Translation;
	AnimBlock Rotation;  // these are shorts in m2 check how rotation is done for bones reading at line 256 as that is shorts too. animblock uses floats
	AnimBlock Scaling;
};

struct VertexColor{
    AnimBlock Colors;
    AnimBlock Alpha;
};


struct M2Light{
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


struct M2Camera{
	u32 Type;    // -1 = flyby, 0 = portrait, 1 = caracter info.  Is both type and index into cam lookup table 
	float FOV;   // multiply by 35 to get degrees
	float FarClip;
	float NearClip;
	AnimBlock CamPosTranslation;
	core::vector3df Position;
	AnimBlock CamTargetTranslation;
	core::vector3df Target;
	AnimBlock Scale;
};


// Structure to contain all arrays from a .skin file
struct SkinData{
	core::array<u16> M2MIndices;
	core::array<u16> M2MTriangles;
	core::array<ModelViewSubmesh> M2MSubmeshes;
	core::array<TextureUnit> M2MTextureUnit;
};


// Structure to teporarily store width & height
struct Bounds{
	float Wmax;
	float Wmin;
	float Hmax;
	float Hmin;
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
	void ReadCameras();
	void ReadUVAnimations();
    void ReadVertices();
    void ReadTextureDefinitions();
    void ReadAnimationData();
    void ReadViewData(io::IReadFile* file);
    void ReadABlock(AnimBlock &ABlock, u8 datatype, u8 datanum);
	void CopyAnimationsToMesh(CM2Mesh * CurrentMesh);
	void BuildANewSubMesh(CM2Mesh * CurrentMesh, u32 v, u32 i, u32 sn); // v is index to current veiw. i is the index to the data for the current submesh in this view. sn is the number of the Submesh we are adding to scene or mesh

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
	core::array<M2Light> M2MLights;
	core::array<M2Camera> M2MCameras;
	core::array<VertexColor> M2MVertexColor;
	core::array<UVAnimation> M2MUVAnimations; // animations for textures
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
		for (int i = 0; i < Submesh.size()-2; i++) 
		{
			for (u16 j = i+1; j < Submesh.size()-1; j++)
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

	void FixDecalDistance (core::array<CM2Mesh::submesh> &Submeshes, core::array<Bounds> &Dimensions)
	{
		for (int i=0; i<Submeshes.size()-1; i++)
		{
			 if (Submeshes[i].Textures[0].shaderType == 2) // if decal
			 {
				s16 index = i-1; // index to previous element
				//s16 InsertHere = index; // index to insert after
				while (index >= 0)
				{
					if(Dimensions[Submeshes[i].SubmeshIndex].Wmax <= Dimensions[Submeshes[index].SubmeshIndex].Wmax && Dimensions[Submeshes[i].SubmeshIndex].Wmin >= Dimensions[Submeshes[index].SubmeshIndex].Wmin && Dimensions[Submeshes[i].SubmeshIndex].Hmax <= Dimensions[Submeshes[index].SubmeshIndex].Hmax && Dimensions[Submeshes[i].SubmeshIndex].Hmin >= Dimensions[Submeshes[i].SubmeshIndex].Hmin)
					{
						// decal distance must be only slightly less than distance of the submesh it falls on
						Submeshes[i].Distance = Submeshes[index].Distance-0.0005;
						index = -1; // end loop if we adjust distance 
					}
					index--;
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
				s16 InsertHere = index; // index to insert after
				while (index >= 0)
				{
					// a decal must be moved between all efects it would be accedentaly projected on and the first none effect element it can project on 
					bool move = false;  // if this becomes true decal will be moved to index+1
					float radius = M2MSkins[skin].M2MSubmeshes[index].Radius;  // AnimatedMesh->SkinID
					//  Only if decal's center of mass is between the other element's top and bottom test for other overlaps
					if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y+radius)
					{ 
						bool overlap = false;
						if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Y < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Y+radius)
						{
							// test x overlap
							if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.X > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.X-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.X < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.X+radius)
							{
								overlap = true;
							}
							// test z overlap;
							if (M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Z > M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Z-radius && M2MSkins[skin].M2MSubmeshes[i].CenterOfMass.Z < M2MSkins[skin].M2MSubmeshes[index].CenterOfMass.Z+radius)
							{
								overlap = true;
							}
						}
						if (overlap == true) // If the decal can project on this element decide if the decal belongs behind or in front of it
						{
							if (Submeshes[index].Textures[0].BlendFlag > 2 && Submeshes[index].Textures[0].shaderType != 2) // if the element is an effect but not a decal the decal must go beneath it.
							{
								InsertHere = index;   // pushes the effect down 1 index and takes over its original index
								move = true;
							}
							if (Submeshes[index].Textures[0].BlendFlag < 2) // if this is not an effect and the decal projects on it then we found the last possible position of this decal
							{
								InsertHere = index+1; // inserts decal at the index folowing this element
								move = true;
								// since we found the last posible location of this decal we need to end the loop so we can find the next decal
								index = 0; 
							}
						}
					}
					if (move == true)
					{
						CM2Mesh::submesh temp = Submeshes[i];
						Submeshes.erase(i);
						Submeshes.insert(temp, InsertHere);
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
