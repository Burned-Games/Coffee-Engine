#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <tracy/Tracy.hpp>

namespace Coffee {

    Shader::Shader(const std::filesystem::path& shaderPath)
        : Resource(ResourceType::Shader)
    {
        ZoneScoped;

        InitializeShader(shaderPath);
    }

    Shader::Shader(const std::string& name, const std::string& shaderSource)
        : Resource(ResourceType::Shader)
    {
        m_Name = name;

        CompileShader(shaderSource);
    }

    Shader::Shader(ImportData& importData)
        : Resource(ResourceType::Shader)
    {
        if(importData.IsValid())
        {
            m_FilePath = importData.originalPath;
            InitializeShader(importData.originalPath);
            m_UUID = importData.uuid;
        }
        else
        {
            m_FilePath = importData.originalPath;
            InitializeShader(importData.originalPath);
            importData.uuid = m_UUID;
        }
    }

    Shader::~Shader()
    {
        ZoneScoped;

        glDeleteProgram(m_ShaderID);
    }

    void Shader::Bind()
    {
        ZoneScoped;

        glUseProgram(m_ShaderID);
    }

    void Shader::Unbind()
    {
        ZoneScoped;

        glUseProgram(0);
    }

    void Shader::setBool(const std::string& name, bool value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform1i(location, (int)value);
    }

    void Shader::setInt(const std::string& name, int value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform1i(location, value);
    }

    void Shader::setFloat(const std::string& name, float value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform1f(location, value);
    }

    void Shader::setVec2(const std::string& name, const glm::vec2& value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform2fv(location, 1, &value[0]);
    }

    void Shader::setVec3(const std::string& name, const glm::vec3& value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform3fv(location, 1, &value[0]);
    }

    void Shader::setVec4(const std::string& name, const glm::vec4& value) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniform4fv(location, 1, &value[0]);
    }

    void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniformMatrix2fv(location, 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniformMatrix3fv(location, 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniformMatrix4fv(location, 1, GL_FALSE, &mat[0][0]);
    }

    void Shader::setMat4v(const std::string& name, const std::vector<glm::mat4>& matrices) const
    {
        ZoneScoped;

        GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
        glUniformMatrix4fv(location, matrices.size(), GL_FALSE, &matrices[0][0][0]);
    }

    void Shader::Recompile()
    {
        ZoneScoped;

        std::string shaderCode = ReadShaderFile(m_FilePath);
        CompileShader(shaderCode);
    }

    Ref<Shader> Shader::Create(const std::filesystem::path& shaderPath)
    {
        ZoneScoped;

        return ResourceLoader::Load<Shader>(shaderPath);
    }

    /*Ref<Shader> Shader::Create(const std::string& shaderSource)
    {
        return ResourceLoader::LoadShader(shaderSource);
    }*/

    void Shader::checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                //std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                COFFEE_CORE_ERROR("ERROR::SHADER_COMPILATION_ERROR of type: {0}\n{1}\n", type, infoLog);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                //std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                COFFEE_CORE_ERROR("ERROR::PROGRAM_LINKING_ERROR of type: {0}\n{1}\n", type, infoLog);
            }
        }
    }

    std::string Shader::ReadShaderFile(const std::filesystem::path& shaderPath)
    {
        std::string shaderCode;
        std::ifstream shaderFile;

        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            shaderFile.open(shaderPath);
            std::stringstream shaderStream;
            shaderStream << shaderFile.rdbuf();
            shaderFile.close();
            shaderCode = shaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            COFFEE_CORE_ERROR(std::string("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: ") + e.what());
        }

        return shaderCode;
    }

    void Shader::CompileShader(const std::string& shaderSource)
    {
        const std::string vertexDelimiter = "#[vertex]";
        const std::string fragmentDelimiter = "#[fragment]";

        size_t vertexPos = shaderSource.find(vertexDelimiter);
        size_t fragmentPos = shaderSource.find(fragmentDelimiter);

        if(vertexPos == std::string::npos || fragmentPos == std::string::npos)
        {
            COFFEE_CORE_ERROR("ERROR::SHADER::DELIMITER_NOT_FOUND: Delimiter not found in shader file!");
            return;
        }

        std::string vertexCode = shaderSource.substr(vertexPos + vertexDelimiter.length(), fragmentPos - vertexPos - vertexDelimiter.length());
        std::string fragmentCode = shaderSource.substr(fragmentPos + fragmentDelimiter.length(), shaderSource.length() - fragmentPos - fragmentDelimiter.length());

        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        m_ShaderID = glCreateProgram();
        glAttachShader(m_ShaderID, vertex);
        glAttachShader(m_ShaderID, fragment);
        glLinkProgram(m_ShaderID);
        checkCompileErrors(m_ShaderID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void Shader::InitializeShader(const std::filesystem::path& shaderPath)
    {
        m_Name = shaderPath.filename().string();
        std::string shaderCode = ReadShaderFile(shaderPath);
        CompileShader(shaderCode);
    }

}
