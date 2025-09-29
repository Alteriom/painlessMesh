import type {SidebarsConfig} from '@docusaurus/plugin-content-docs';

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.
 */
const sidebars: SidebarsConfig = {
  // painlessMesh documentation sidebar
  tutorialSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Getting Started',
      items: [
        'getting-started/installation',
        'getting-started/quickstart', 
        'getting-started/first-mesh',
      ],
    },
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api/core-api',
        'api/callbacks',
        'api/configuration',
      ],
    },
    {
      type: 'category',
      label: 'Alteriom Extensions',
      items: [
        'alteriom/packages',
        'alteriom/sensor-networks',
        'alteriom/examples',
      ],
    },
    {
      type: 'category',
      label: 'Tutorials',
      items: [
        'tutorials/basic-examples',
        'tutorials/custom-packages',
        'tutorials/sensor-networks',
        'tutorials/home-automation',
      ],
    },
    {
      type: 'category',
      label: 'Architecture',
      items: [
        'architecture/mesh-architecture',
        'architecture/plugin-system',
        'architecture/routing',
      ],
    },
    {
      type: 'category',
      label: 'Advanced',
      items: [
        'advanced/performance',
        'advanced/memory-management',
        'advanced/troubleshooting',
      ],
    },
    {
      type: 'category',
      label: 'Troubleshooting',
      items: [
        'troubleshooting/faq',
        'troubleshooting/common-issues',
        'troubleshooting/debugging',
      ],
    },
  ],
};

export default sidebars;
