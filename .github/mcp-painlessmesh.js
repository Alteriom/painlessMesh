#!/usr/bin/env node

/**
 * Custom MCP Server for painlessMesh Development
 * Provides specialized tools for working with the painlessMesh library
 */

const { Server } = require('@modelcontextprotocol/sdk/server/index.js');
const { StdioServerTransport } = require('@modelcontextprotocol/sdk/server/stdio.js');
const { CallToolRequestSchema, ListToolsRequestSchema } = require('@modelcontextprotocol/sdk/types.js');
const fs = require('fs').promises;
const path = require('path');
const { spawn } = require('child_process');

const PROJECT_ROOT = process.env.PROJECT_ROOT || '/home/runner/work/painlessMesh/painlessMesh';

class PainlessMeshMCPServer {
  constructor() {
    this.server = new Server(
      {
        name: 'painlessmesh-dev',
        version: '1.0.0',
      },
      {
        capabilities: {
          tools: {},
        },
      }
    );

    this.setupToolHandlers();
  }

  setupToolHandlers() {
    this.server.setRequestHandler(ListToolsRequestSchema, async () => ({
      tools: [
        {
          name: 'build_project',
          description: 'Build the painlessMesh project using CMake and Ninja',
          inputSchema: {
            type: 'object',
            properties: {
              clean: {
                type: 'boolean',
                description: 'Whether to clean build artifacts first',
                default: false
              }
            }
          }
        },
        {
          name: 'run_tests',
          description: 'Run painlessMesh test suites',
          inputSchema: {
            type: 'object',
            properties: {
              test_pattern: {
                type: 'string',
                description: 'Pattern to match specific tests (e.g., "catch_plugin", "catch_*")',
                default: 'catch_*'
              },
              verbose: {
                type: 'boolean',
                description: 'Enable verbose test output',
                default: false
              }
            }
          }
        },
        {
          name: 'analyze_mesh_code',
          description: 'Analyze painlessMesh source code structure and dependencies',
          inputSchema: {
            type: 'object',
            properties: {
              component: {
                type: 'string',
                description: 'Specific component to analyze (mesh, tcp, protocol, plugin, router)',
                enum: ['mesh', 'tcp', 'protocol', 'plugin', 'router', 'all']
              }
            }
          }
        },
        {
          name: 'create_alteriom_package',
          description: 'Generate template for a new Alteriom-specific package',
          inputSchema: {
            type: 'object',
            properties: {
              package_name: {
                type: 'string',
                description: 'Name of the new package (e.g., "SensorData", "CommandPackage")'
              },
              package_type: {
                type: 'string',
                description: 'Type of package to create',
                enum: ['single', 'broadcast']
              },
              fields: {
                type: 'array',
                description: 'Array of field definitions {name, type, description}',
                items: {
                  type: 'object',
                  properties: {
                    name: { type: 'string' },
                    type: { type: 'string' },
                    description: { type: 'string' }
                  }
                }
              }
            },
            required: ['package_name', 'package_type']
          }
        },
        {
          name: 'validate_dependencies',
          description: 'Check and validate project dependencies and submodules',
          inputSchema: {
            type: 'object',
            properties: {}
          }
        }
      ],
    }));

    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      const { name, arguments: args } = request.params;

      try {
        switch (name) {
          case 'build_project':
            return await this.buildProject(args);
          case 'run_tests':
            return await this.runTests(args);
          case 'analyze_mesh_code':
            return await this.analyzeMeshCode(args);
          case 'create_alteriom_package':
            return await this.createAlteriomPackage(args);
          case 'validate_dependencies':
            return await this.validateDependencies();
          default:
            throw new Error(`Unknown tool: ${name}`);
        }
      } catch (error) {
        return {
          content: [
            {
              type: 'text',
              text: `Error executing ${name}: ${error.message}`,
            },
          ],
          isError: true,
        };
      }
    });
  }

  async buildProject(args) {
    const { clean = false } = args;
    
    try {
      let output = '';
      
      if (clean) {
        await this.executeCommand('rm', ['-rf', 'CMakeCache.txt', 'CMakeFiles', 'bin', 'build.ninja'], PROJECT_ROOT);
        output += 'Cleaned build artifacts\n';
      }
      
      // Configure with CMake
      const configResult = await this.executeCommand('cmake', ['-G', 'Ninja', '.'], PROJECT_ROOT);
      output += `CMake configuration:\n${configResult}\n`;
      
      // Build with Ninja
      const buildResult = await this.executeCommand('ninja', [], PROJECT_ROOT);
      output += `Build result:\n${buildResult}\n`;
      
      return {
        content: [
          {
            type: 'text',
            text: `Build completed successfully:\n${output}`,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Build failed: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async runTests(args) {
    const { test_pattern = 'catch_*', verbose = false } = args;
    
    try {
      const binDir = path.join(PROJECT_ROOT, 'bin');
      const files = await fs.readdir(binDir);
      const testFiles = files.filter(file => {
        if (test_pattern === 'catch_*') {
          return file.startsWith('catch_');
        }
        return file.includes(test_pattern);
      });
      
      let output = `Found ${testFiles.length} test files matching "${test_pattern}":\n`;
      
      for (const testFile of testFiles) {
        const testPath = path.join(binDir, testFile);
        try {
          const result = await this.executeCommand(testPath, verbose ? ['--verbose'] : [], PROJECT_ROOT);
          output += `\n=== ${testFile} ===\n${result}\n`;
        } catch (error) {
          output += `\n=== ${testFile} FAILED ===\n${error.message}\n`;
        }
      }
      
      return {
        content: [
          {
            type: 'text',
            text: output,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Test execution failed: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async analyzeMeshCode(args) {
    const { component = 'all' } = args;
    
    try {
      const srcDir = path.join(PROJECT_ROOT, 'src', 'painlessmesh');
      let analysis = 'painlessMesh Code Analysis:\n\n';
      
      const components = component === 'all' 
        ? ['mesh', 'tcp', 'protocol', 'plugin', 'router', 'logger']
        : [component];
      
      for (const comp of components) {
        const filePath = path.join(srcDir, `${comp}.hpp`);
        try {
          const content = await fs.readFile(filePath, 'utf-8');
          analysis += `=== ${comp}.hpp ===\n`;
          
          // Extract classes and namespaces
          const classes = content.match(/class\s+(\w+)/g) || [];
          const namespaces = content.match(/namespace\s+(\w+)/g) || [];
          const templates = content.match(/template\s*<[^>]*>\s*\w+/g) || [];
          
          analysis += `Classes: ${classes.join(', ')}\n`;
          analysis += `Namespaces: ${namespaces.join(', ')}\n`;
          analysis += `Templates: ${templates.length}\n`;
          analysis += `Lines of code: ${content.split('\n').length}\n\n`;
          
        } catch (error) {
          analysis += `${comp}.hpp: Not found or error reading\n\n`;
        }
      }
      
      return {
        content: [
          {
            type: 'text',
            text: analysis,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Code analysis failed: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async createAlteriomPackage(args) {
    const { package_name, package_type, fields = [] } = args;
    
    const baseClass = package_type === 'single' ? 'plugin::SinglePackage' : 'plugin::BroadcastPackage';
    const typeId = Math.floor(Math.random() * 1000) + 200; // Random ID starting from 200
    
    let packageCode = `#ifndef ALTERIOM_${package_name.toUpperCase()}_HPP
#define ALTERIOM_${package_name.toUpperCase()}_HPP

#include "painlessmesh/plugin.hpp"

namespace alteriom {

class ${package_name} : public ${baseClass} {
public:
`;

    // Add field declarations
    fields.forEach(field => {
      packageCode += `    ${field.type} ${field.name}; // ${field.description || ''}\n`;
    });

    packageCode += `
    ${package_name}() : ${baseClass}(${typeId}) {}
    
    ${package_name}(JsonObject jsonObj) : ${baseClass}(jsonObj) {
`;

    // Add JSON deserialization
    fields.forEach(field => {
      packageCode += `        ${field.name} = jsonObj["${field.name}"];\n`;
    });

    packageCode += `    }
    
    JsonObject addTo(JsonObject&& jsonObj) const {
        jsonObj = ${baseClass}::addTo(std::move(jsonObj));
`;

    // Add JSON serialization
    fields.forEach(field => {
      packageCode += `        jsonObj["${field.name}"] = ${field.name};\n`;
    });

    packageCode += `        return jsonObj;
    }

#if ARDUINOJSON_VERSION_MAJOR < 7
    size_t jsonObjectSize() const { 
        return JSON_OBJECT_SIZE(noJsonFields + ${fields.length}); 
    }
#endif
};

} // namespace alteriom

#endif // ALTERIOM_${package_name.toUpperCase()}_HPP
`;

    // Also create a test template
    const testCode = `#include "catch2/catch.hpp"
#include "alteriom_${package_name.toLowerCase()}.hpp"

using namespace alteriom;

SCENARIO("${package_name} serialization works correctly") {
    GIVEN("A ${package_name} with test data") {
        auto pkg = ${package_name}();
        pkg.from = 1;
${package_type === 'single' ? '        pkg.dest = 2;' : ''}
${fields.map(field => `        pkg.${field.name} = /* test value */;`).join('\n')}
        
        WHEN("Converting it to and from Variant") {
            auto var = protocol::Variant(&pkg);
            auto pkg2 = var.to<${package_name}>();
            
            THEN("Should result in the same values") {
                REQUIRE(pkg2.from == pkg.from);
${package_type === 'single' ? '                REQUIRE(pkg2.dest == pkg.dest);' : ''}
${fields.map(field => `                REQUIRE(pkg2.${field.name} == pkg.${field.name});`).join('\n')}
                REQUIRE(pkg2.type == pkg.type);
            }
        }
    }
}
`;

    return {
      content: [
        {
          type: 'text',
          text: `Generated Alteriom package template for ${package_name}:\n\n=== Header File (alteriom_${package_name.toLowerCase()}.hpp) ===\n${packageCode}\n\n=== Test File (test_alteriom_${package_name.toLowerCase()}.cpp) ===\n${testCode}`,
        },
      ],
    };
  }

  async validateDependencies() {
    try {
      let report = 'Dependency Validation Report:\n\n';
      
      // Check submodules
      const testDir = path.join(PROJECT_ROOT, 'test');
      const requiredDirs = ['ArduinoJson', 'TaskScheduler'];
      
      for (const dir of requiredDirs) {
        const dirPath = path.join(testDir, dir);
        try {
          await fs.access(dirPath);
          report += `✓ ${dir}: Present\n`;
        } catch {
          report += `✗ ${dir}: Missing\n`;
        }
      }
      
      // Check build tools
      const tools = [
        { name: 'cmake', args: ['--version'] },
        { name: 'ninja', args: ['--version'] },
        { name: 'g++', args: ['--version'] }
      ];
      
      report += '\nBuild Tools:\n';
      for (const tool of tools) {
        try {
          await this.executeCommand(tool.name, tool.args, PROJECT_ROOT);
          report += `✓ ${tool.name}: Available\n`;
        } catch {
          report += `✗ ${tool.name}: Not available\n`;
        }
      }
      
      // Check library files
      const libFiles = [
        'src/painlessmesh/mesh.hpp',
        'src/painlessmesh/tcp.hpp',
        'src/painlessmesh/protocol.hpp',
        'src/painlessmesh/plugin.hpp'
      ];
      
      report += '\nCore Library Files:\n';
      for (const file of libFiles) {
        const filePath = path.join(PROJECT_ROOT, file);
        try {
          await fs.access(filePath);
          report += `✓ ${file}: Present\n`;
        } catch {
          report += `✗ ${file}: Missing\n`;
        }
      }
      
      return {
        content: [
          {
            type: 'text',
            text: report,
          },
        ],
      };
    } catch (error) {
      return {
        content: [
          {
            type: 'text',
            text: `Dependency validation failed: ${error.message}`,
          },
        ],
        isError: true,
      };
    }
  }

  async executeCommand(command, args, cwd) {
    return new Promise((resolve, reject) => {
      const process = spawn(command, args, { cwd, stdio: 'pipe' });
      
      let stdout = '';
      let stderr = '';
      
      process.stdout.on('data', (data) => {
        stdout += data.toString();
      });
      
      process.stderr.on('data', (data) => {
        stderr += data.toString();
      });
      
      process.on('close', (code) => {
        if (code === 0) {
          resolve(stdout + stderr);
        } else {
          reject(new Error(`Command failed with code ${code}: ${stderr}`));
        }
      });
      
      process.on('error', (error) => {
        reject(error);
      });
    });
  }

  async run() {
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
    console.error('painlessMesh MCP server running on stdio');
  }
}

if (require.main === module) {
  const server = new PainlessMeshMCPServer();
  server.run().catch(console.error);
}

module.exports = PainlessMeshMCPServer;