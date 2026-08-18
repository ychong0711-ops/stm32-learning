# Pull Request Template

## Description
Please describe the changes you've made and why.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Documentation update
- [ ] Workflow/CI update
- [ ] Refactoring
- [ ] Other

## Checklist
- [ ] I have read the CONTRIBUTING guidelines
- [ ] My code follows the project's style guidelines
- [ ] I have performed a self-review of my code
- [ ] I have updated documentation as needed
- [ ] My changes generate no new warnings (if applicable)
- [ ] I have added/updated tests as needed

## RTA/Build Validation
Since this project includes RTA analysis and embedded code quality checks:
- [ ] `python3 rta_analysis.py` runs without errors
- [ ] GCC syntax check passes: `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -DSTM32F446xx main_fixed.c -fsyntax-only`
- [ ] Static analysis considerations noted

## Screenshots (if applicable)
Add screenshots to help explain your changes.

## Checklist for Learning Program Changes
- [ ] Code learning section updated if applicable
- [ ] Quiz missions updated if applicable
- [ ] Path/course changes reviewed