#pragma once
#include <GL/glew.h>
#include <string>
#include <deque>
#include "Vao.h"

#include "detail/Shader/Shaders.h"
#include "detail/Uniform/Subroutines.h"

namespace df
{
struct SetSubroutinesType {}; // TODO remove if possible

struct _CompShader { const char* path; };
struct _FragShader { const char* path; };
struct _VertShader { const char* path; };
struct _GeomShader { const char* path; };
struct _TescShader { const char* path; };
struct _TeseShader { const char* path; };

class ProgramLowLevelBase;
template<typename Uniform_T> class ProgramBase;

template<
	typename Shaders_T,
	typename Uni_T,					// Class implementing setting uniforms and rendering them
	typename Subroutines_T = Subroutines<Shaders_T>>
class Program :
	protected ProgramBase<Uni_T>
{	//static_assert(std::is_base_of_v<FragComp_t, Shader<SFile>>, "Must have a proper fragment/compute shader type set");
	class LoadState;	//fwd decl
	LoadState load_state = LoadState(*this);

protected:
	using Comp_t = typename Shaders_T::Comp;
	using Frag_t = typename Shaders_T::Frag;
	using Vert_t = typename Shaders_T::Vert;
	using Geom_t = typename Shaders_T::Geom;
	using TesC_t = typename Shaders_T::TesC;
	using TesE_t = typename Shaders_T::TesE;
	
public:
	Program(const std::string& name = ""); //glCreateProgram :)
	Program(const char* name = "");
	~Program() = default;

	bool Link();
	const std::string& GetErrors() const { return this->getErrors(); }
	
	//For pushing uniforms
	typename ProgramBase<Uni_T>::InvalidState& operator << (const std::string &str);

	//For rendering
	inline Program& operator << (const VaoBase& vao) { *this << SetSubroutinesType(); this->draw(vao); return *this;
}

	//For adding shader files. Same types will concatenate.
	inline LoadState& operator << (const _CompShader& s){ return (this->load_state << s); }
	inline LoadState& operator << (const _FragShader& s){ return (this->load_state << s); }
	inline LoadState& operator << (const _VertShader& s){ return (this->load_state << s); }
	inline LoadState& operator << (const _GeomShader& s){ return (this->load_state << s); }
	inline LoadState& operator << (const _TescShader& s){ return (this->load_state << s); }
	inline LoadState& operator << (const _TeseShader& s){ return (this->load_state << s); }
	//Compile shaders and Link the program
	inline LoadState& operator << (const typename LoadState::LinkType link) { return (this->load_state << link); }
	//Set subroutines on GPU, has to be called before every draw, TODO remove function and call SetSubroutines automatically on rendering
	inline Program& operator << (const SetSubroutinesType& set) { subroutines.SetSubroutines(); return *this; }


	// This will render and update shaders. Only use it with the base program
	void Render() {}
protected:
	Comp_t	comp;
	Frag_t	frag;	Vert_t vert;	Geom_t geom;	TesC_t tesc;	TesE_t tese;
	std::string program_name;
	Subroutines_T subroutines;
};

class Uniforms;

using ShaderProgramVF = Program<ShaderVF, Uniforms>;
using ShaderProgramVGF = Program<ShaderVGF, Uniforms>;
using ShaderProgramVTF = Program<ShaderVTF, Uniforms>;
using ShaderProgramVGTF = Program<ShaderVGTF, Uniforms>;
using ComputeProgram = Program<ShaderCompute, Uniforms>;

} //namespace df

#include "detail/Program/Program.inl"