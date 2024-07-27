def Settings( **kwargs ):
  return {
    'flags': [
        '-Isrc/',
        '-Ilib/glad/include', 
        '-Ilib/stb', 
        '-Ilib/glfw/include', 
        '-Ilib/cglm/include',
        '-Ilib/perlin-noise/src'
    ]
  }
