#pragma once
#include <tuple>
#include "../../config.h"
#include "../Texture/Texture.h"
#include "../Texture/Texture2D.h"
#include "../Texture/TextureCube.h"

namespace df
{

class FrameBufferBase
{

};

class DefaultFramebuffer :public FrameBufferBase
{

};



template<typename InternalFormat = eltecg::ogl::helper::depth24>
class Renderbuffer
{
	GLuint _id;
	bool inited = false;
	
	void bind() { glBindRenderbuffer(GL_RENDERBUFFER, _id); }
public:
	using PixelFormat = InternalFormat;
	Renderbuffer() { glGenRenderbuffers(1, &_id); }
	Renderbuffer(GLsizei w, GLsizei h) { glGenRenderbuffers(1, &_id); Init(w, h); }
	~Renderbuffer() { glDeleteRenderbuffers(1, &_id); }
	
	void Init(GLsizei width, GLsizei height)
	{
		this->bind();
		constexpr GLenum iFormat = eltecg::ogl::helper::getInternalFormat<InternalFormat>();
		glRenderbufferStorage(GL_RENDERBUFFER, iFormat, width, height);
	}
};


namespace detail {

	constexpr unsigned _none_ = std::numeric_limits<unsigned>::max();

	template<unsigned depth_= _none_, unsigned stencil_ = _none_, unsigned depthstencil_ = _none_>
	struct FBO_compile_data{static constexpr unsigned depth() { return depth_; }; static constexpr unsigned stencil() { return stencil_; } static constexpr unsigned depthstencil() { return depthstencil_; } };
	namespace cg = eltecg::ogl::helper;

	template<typename F> constexpr bool has_depth_v				= std::is_same_v<F, cg::depth16> || std::is_same_v<F, cg::depth24> || std::is_same_v<F, cg::depth32F>;
	template<typename F> constexpr bool has_stencil_v			= std::is_same_v<F, cg::stencil1> || std::is_same_v<F, cg::stencil4> || std::is_same_v<F, cg::stencil8> || std::is_same_v<F, cg::stencil16>;
	template<typename F> constexpr bool has_depthstencil_v		= std::is_same_v<F, cg::depth24stencil8> || std::is_same_v<F, cg::depth32Fstencil8 >;
	template<typename F> constexpr bool is_color_attachement_v	= !has_depth_v<F> && !has_stencil_v<F> && !has_depthstencil_v<F>;
	template<typename F> constexpr GLenum get_attachement_v		= has_depth_v<F> ? GL_DEPTH_ATTACHMENT : has_stencil_v<F> ? GL_STENCIL_ATTACHMENT : has_depthstencil_v<F> ? GL_DEPTH_STENCIL_ATTACHMENT : 0;

}

template<typename compile_data, typename ... Attachements>
class FramebufferObject : public FrameBufferBase
{
private:
	//namespace cg = eltecg::ogl::helper;
	std::tuple<Attachements...> _attachements;
	GLuint _id;
	GLuint _width = 0, _height = 0;
private:

	void bind() { glBindFramebuffer(GL_FRAMEBUFFER, _id); }

	template<typename F> using FBO_compile_data_add_InternalFormat = detail::FBO_compile_data<
		detail::has_depth_v<F>        ? sizeof...(Attachements) : detail::_none_ ,
		detail::has_stencil_v<F>      ? sizeof...(Attachements) : detail::_none_ ,
		detail::has_depthstencil_v<F> ? sizeof...(Attachements) : detail::_none_ >;
	template<typename F> using FramebufferObject_add_Texture2D    = FramebufferObject < FBO_compile_data_add_InternalFormat<F>, Attachements..., Texture2D<F>>;
	template<typename F> using FramebufferObject_add_Renderbuffer = FramebufferObject < FBO_compile_data_add_InternalFormat<F>, Attachements..., Renderbuffer<F>>;

	template<int idx>
	static constexpr int calc_extra_spots_until()
	{
		static_assert(idx >= 0 && idx <= sizeof...(Attachements), "Index out of bounds here.");
		return (idx > compile_data::depth() ? 1 : 0) + (idx > compile_data::stencil() ? 1 : 0) + (idx > compile_data::depthstencil() ? 1 : 0);
	}

public:
	FramebufferObject(GLuint id, std::tuple<Attachements...> &&attachements, GLuint width = 0, GLuint height = 0):_id(id), _attachements(std::move(attachements)), _width(width), _height(height){}
	FramebufferObject() { glCreateFramebuffers(1, &_id); }
	~FramebufferObject() { if(_id != 0) glDeleteFramebuffers(1, &_id); }
	FramebufferObject(FramebufferObject&&) = default;
	//FramebufferObject& operator=(FramebufferObject&&) = default;


	template<typename InternalFormat>
	FramebufferObject_add_Texture2D<InternalFormat> operator + (const Texture2D<InternalFormat> &tex)
	{
		static_assert(compile_data::depth()        == detail::_none_ || !detail::has_depth_v<InternalFormat>   && !detail::has_depthstencil_v<InternalFormat>, "An FBO cannot have two depth textures."  );
		static_assert(compile_data::stencil()      == detail::_none_ || !detail::has_stencil_v<InternalFormat> && !detail::has_depthstencil_v<InternalFormat>, "An FBO cannot have two stencil textures.");
		static_assert(compile_data::depthstencil() == detail::_none_ || !detail::has_depthstencil_v<InternalFormat>, "An FBO cannot have two depth-stencil textures, that is just too much!");

		ASSERT((this->_width == 0 || tex.getWidth() == 0 || this->_width == tex.getWidth()) && (this->_height == 0 || tex.getHeight() == 0 || this->_height == tex.getHeight()), "Unmaching texture size in framebuffer object.");

		int w = (this->_width  == 0 ? tex.getWidth()  : this->_width), h = (this->_height == 0 ? tex.getHeight() : this->_height);

		FramebufferObject_add_Texture2D<InternalFormat> fbo(this->_id, std::tuple_cat(std::move(this->_attachements), std::make_tuple(tex.MakeView(0_level))), w, h);
		this->bind(); //same as fbo.bind() but available
		
		constexpr int idx = sizeof...(Attachements);
		constexpr int index = idx - calc_extra_spots_until<idx>();
		static_assert(index >= 0 && index <= sizeof...(Attachements), "Index out of bounds here.");
		constexpr GLenum attachement = detail::is_color_attachement_v<InternalFormat> ? GL_COLOR_ATTACHMENT0 + index : detail::get_attachement_v<InternalFormat>;

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, tex.GetID(), 0);

		this->_id = 0;
		return fbo;
	}

	template<int idx>
	constexpr typename std::tuple_element<idx, std::tuple<Attachements...>>& get()
	{
		constexpr int index = idx + calc_extra_spots_until<idx>();
		static_assert(index >= 0 && index < sizeof...(Attachements), "Index out of bounds here.");
		return std::get< index >(_attachements);
	}
};

using EmptyFBO = FramebufferObject<detail::FBO_compile_data<>>;

template<typename ... ColorAttachements>
using FBO = FramebufferObject<Renderbuffer<>, ColorAttachements...>;

}

