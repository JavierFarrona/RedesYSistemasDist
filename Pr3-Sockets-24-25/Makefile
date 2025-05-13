# Makefile fusionado: funcionalidades del usuario y generador

CXX = g++
CXXFLAGS = -g -std=gnu++0x
LDFLAGS =
TARGET = ftp_server

# Carpetas
SRC_DIR = src
LIB_DIR = lib
OBJ_DIR = trash

# Crear carpeta para los archivos objeto si no existe
$(shell mkdir -p $(OBJ_DIR))
# Archivos fuente y objetos
SRC = $(wildcard $(SRC_DIR)/*.cpp)
LIB = $(wildcard $(LIB_DIR)/*.h)
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

# Compilación del proyecto
all: build

build: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

# Reglas para compilar cada archivo fuente a objeto
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpieza de archivos generados
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Eliminación del archivo comprimido
tar_clean:
	@if [ -z "$(DIR)" ]; then \
	    echo "Error: Debes especificar un directorio usando 'make tar_clean DIR=<directorio>'"; \
	    exit 1; \
	fi
	rm -f "$(DIR).tgz"

# Formateo de archivos según la guía de estilo de Google
format:
	@files=$$(find $(SRC_DIR) $(LIB_DIR) -type f \( -name '*.cpp' -o -name '*.h' \)); \
	if [ -n "$$files" ]; then \
	    echo "Formateando los siguientes archivos:"; \
	    echo "$$files"; \
	    clang-format -i --style=Google $$files; \
	else \
	    echo "No hay archivos para formatear en $(SRC_DIR) o $(LIB_DIR)."; \
	fi

# Creación de un archivo comprimido de un directorio específico
tar:
	@read -p "Introduce el directorio a comprimir: " dir; \
	if [ ! -d "$$dir" ]; then \
	    echo "Error: El directorio $$dir no existe."; \
	    exit 1; \
	fi; \
	BASE_DIR=$$(dirname "$$dir"); \
	ARCHIVE_NAME=$$(basename "$$dir").tgz; \
	echo "Creating archive $$BASE_DIR/$$ARCHIVE_NAME"; \
	tar --exclude="$$dir/$$ARCHIVE_NAME" -czvf "$$BASE_DIR/$$ARCHIVE_NAME" "$$dir"

# Commit y push a git con un mensaje personalizado
git:
	@read -p "Introduce el mensaje del commit: " msg; \
	git add .; \
	git commit -m "$$msg"; \
	git push

# Crear un archivo README.md con detalles de la práctica
readme:
	@read -p "Nombre de la práctica: " practica; \

	read -p "Asignatura: " asignatura; \

	read -p "Curso: " curso; \

	read -p "Cuatrimestre: " cuatri; \

	read -p "Autor: " autor; \

	echo "# 🧪 Práctica $$practica" > README.md; \
	echo "**Asignatura:** $$asignatura" >> README.md; \
	echo "**Grado:** Nombre del grado o carrera" >> README.md; \
	echo "**Curso:** $$curso" >> README.md; \
	echo "**Cuatrimestre:** $$cuatri" >> README.md; \
	echo "**Autor:** $$autor" >> README.md; \
	echo "" >> README.md; \
	echo "---" >> README.md; \
	echo "## 📄 Descripción" >> README.md; \
	echo "Breve descripción de la práctica y sus objetivos." >> README.md; \
	echo "" >> README.md; \
	echo "---" >> README.md; \
	echo "## 📁 Estructura del proyecto" >> README.md; \
	echo "- `src/`: Archivos fuente (.cpp)" >> README.md; \
	echo "- `lib/`: Archivos de cabecera (.h)" >> README.md; \
	echo "- `trash/`: Archivos objeto temporales (.o)" >> README.md; \
	echo "- `Makefile`: Script de construcción" >> README.md; \
	echo "- `README.md`: Este documento" >> README.md; \
	echo "" >> README.md; \
	echo "---" >> README.md; \
	echo "## ⚙️ Compilación y ejecución" >> README.md; \
	echo '```bash' >> README.md; \
	echo "make       # Compila el proyecto" >> README.md; \
	echo "./si       # Ejecuta el binario generado" >> README.md; \
	echo '```' >> README.md; \
	echo "" >> README.md; \
	echo "---" >> README.md; \
	echo "## 🔍 Detalles técnicos" >> README.md; \
	echo "- Lenguaje: C++" >> README.md; \
	echo "- Estándar: C++17" >> README.md; \
	echo "- Compilador: g++" >> README.md; \
	echo "- Formato: Google Style (clang-format)" >> README.md; \
	echo "" >> README.md; \
	echo "---" >> README.md; \
	echo "## 📝 Conclusiones" >> README.md; \
	echo "Aquí puedes escribir reflexiones personales, problemas encontrados o ideas de mejora." >> README.md; \
	echo "" >> README.md; \
	echo "README.md generado correctamente." >> README.md
