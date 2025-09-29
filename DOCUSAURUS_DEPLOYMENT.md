# GitHub Pages Deployment Guide

This guide explains how to deploy the Docusaurus documentation site to GitHub Pages.

## 🚀 Deployment Steps

### 1. Enable GitHub Pages

1. Go to your repository on GitHub: `https://github.com/Alteriom/painlessMesh`
2. Click **Settings** tab
3. Scroll down to **Pages** section
4. Under **Source**, select **GitHub Actions**
5. Save the changes

### 2. Commit and Push Changes

```bash
# Add all changes
git add .

# Commit the Docusaurus setup
git commit -m "feat: Add Docusaurus documentation site

- Replace basic HTML generation with modern Docusaurus
- Add GitHub Actions workflow for automated deployment
- Include Alteriom package documentation
- Integrate Doxygen API docs with user guides
- Enable responsive design and built-in search

Closes: Documentation modernization initiative"

# Push to trigger deployment
git push origin main
```

### 3. Monitor Deployment

1. Go to **Actions** tab in your repository
2. Watch the **Documentation** workflow run
3. When complete, your site will be available at:
   - **URL**: `https://alteriom.github.io/painlessMesh/`

## 🛠️ Local Development

### Start Development Server

```bash
cd website
npm start
```

Visit: `http://localhost:3000/painlessMesh/`

### Build for Production

```bash
cd website
npm run build
```

### Test Production Build

```bash
cd website
npm run serve
```

## 📁 Documentation Structure

```
website/
├── docs/                          # Main documentation
│   ├── intro.md                   # Homepage content
│   ├── getting-started/           # Installation & quickstart
│   ├── api/                       # API reference
│   ├── alteriom/                  # Alteriom extensions
│   ├── tutorials/                 # Usage examples
│   ├── architecture/              # Technical details
│   ├── advanced/                  # Advanced topics
│   └── troubleshooting/           # Help & FAQ
├── static/                        # Static assets
│   └── api/                       # Doxygen API docs (auto-generated)
├── docusaurus.config.ts          # Main configuration
└── sidebars.ts                   # Navigation structure
```

## 🔄 Workflow Overview

The GitHub Actions workflow:

1. **Checkout** repository code
2. **Setup Node.js** for Docusaurus
3. **Install Doxygen** for API documentation
4. **Generate API docs** using existing Doxygen config
5. **Install dependencies** for Docusaurus
6. **Integrate Doxygen** output with Docusaurus
7. **Build site** for production
8. **Deploy** to GitHub Pages

## 🎯 Benefits Over Previous System

| Feature | Old System | **New Docusaurus** |
|---------|------------|-------------------|
| **Search** | ❌ None | ✅ Built-in Algolia search |
| **Mobile** | ❌ Poor responsive | ✅ Perfect mobile experience |
| **Navigation** | ❌ Manual links | ✅ Auto-generated sidebar |
| **Performance** | ❌ Slow page loads | ✅ Single-page app speed |
| **Maintenance** | ❌ Manual HTML generation | ✅ Pure Markdown workflow |
| **Link validation** | ❌ Broken links undetected | ✅ Automatic validation |
| **Versioning** | ❌ Not supported | ✅ Multiple library versions |
| **API integration** | ❌ Separate Doxygen site | ✅ Seamless integration |

## 🔧 Customization

### Adding New Pages

1. Create `.md` files in appropriate `docs/` subdirectory
2. Update `sidebars.ts` to include in navigation
3. Commit and push - automatic deployment

### Modifying Branding

Edit `docusaurus.config.ts`:
- `title`: Site title
- `tagline`: Site description  
- `favicon`: Icon file
- `themeConfig.navbar`: Navigation menu
- `themeConfig.footer`: Footer content

### Custom Styling

Edit `src/css/custom.css` for custom styles and branding.

## 🚨 Troubleshooting

### Build Fails

1. Check **Actions** tab for error details
2. Verify all referenced files exist in sidebars
3. Ensure Markdown syntax is valid

### Pages Not Deploying

1. Verify **GitHub Pages** is set to **GitHub Actions**
2. Check repository permissions
3. Ensure workflow has **Pages write** permission

### Links Broken

1. Use relative paths: `../other-page`
2. Verify file extensions: `.md` files become `.html`
3. Check sidebar configuration matches file structure

## 📞 Support

For issues with:
- **Docusaurus**: See [Docusaurus docs](https://docusaurus.io/)
- **GitHub Actions**: Check workflow logs in Actions tab
- **Content**: Create issues in repository

## 🎉 Next Steps

1. **Enable search**: Configure Algolia search index
2. **Add analytics**: Integrate Google Analytics
3. **Custom domain**: Set up custom domain if desired
4. **Content migration**: Move remaining docs from `/docs` folder