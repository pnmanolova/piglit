/*
 * Copyright © 2013 Chris Forbes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

    config.supports_gl_compat_version = 30;

    config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;
    config.khr_no_error_support = PIGLIT_NO_ERRORS;

PIGLIT_GL_TEST_CONFIG_END

/* test execution of sample masking.
 * - set mask to half the samples
 * - render a red thing
 * - set mask to the other half of the samples
 * - render a green thing
 *
 * - blit from the MSAA buffer to the single-sampled FBO
 * - blit from the single-sampled FBO to the winsys buffer
 * - ensure that the pixels are yellow
 *
 *   note the intermediate single-sampled FBO is only necessary so that
 *   the resolve is always happening FBO->FBO; if we resolve into a winsys
 *   buffer, there are sRGB interactions.
 */

GLuint fbo, ss_fbo, tex, ss_rb;

enum piglit_result
piglit_display(void)
{
    float half_yellow[] = { 0.5f, 0.5f, 0.0f, 1.0f };
    bool pass = true;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_SAMPLE_MASK);

    glSampleMaski(0, 0x3);              /* first and second samples */
    glColor4f(1.0, 0.0, 0.0, 1.0);
    piglit_draw_rect(-1,-1,2,2);

    glSampleMaski(0, 0xc);              /* third and fourth samples */
    glColor4f(0.0, 1.0, 0.0, 1.0);
    piglit_draw_rect(-1,-1,2,2);

    glDisable(GL_SAMPLE_MASK);

    if (!piglit_check_gl_error(GL_NO_ERROR))
        piglit_report_result(PIGLIT_FAIL);

    /* resolve */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ss_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);


    /* single-sampled blit */
    glBindFramebuffer(GL_FRAMEBUFFER, piglit_winsys_fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, ss_fbo);
    glBlitFramebuffer(0, 0, 64, 64, 0, 0, 64, 64,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    if (!piglit_check_gl_error(GL_NO_ERROR))
        piglit_report_result(PIGLIT_FAIL);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, piglit_winsys_fbo);

    /* the resolve done by the blit should
     * blend the red and green samples together */
    pass = piglit_probe_pixel_rgba(32, 32, half_yellow) && pass;

    piglit_present_results();

    return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
    bool use_multisample_texture = false;
    GLint max_samples;

    piglit_require_extension("GL_ARB_texture_multisample");

    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
    if (max_samples < 4) {
       /* We need 4x msaa for this test.  Note that GL requires support
	* for at least 4x msaa when msaa is supported.
	*/
       printf("GL_MAX_SAMPLES = %d, need 4\n", max_samples);
       piglit_report_result(PIGLIT_SKIP);
    }

    while (++argv,--argc) {
        if (!strcmp(*argv, "-tex"))
            use_multisample_texture = true;
    }

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    if (use_multisample_texture) {
        /* use a multisample texture */
        printf("Using GL_TEXTURE_2D_MULTISAMPLE\n");

        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, tex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                4, GL_RGBA, 64, 64, GL_TRUE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D_MULTISAMPLE, tex, 0);
    }
    else {
        /* use a classic renderbuffer */
        printf("Using classic MSAA renderbuffer\n");

        glGenRenderbuffers(1, &tex);
        glBindRenderbuffer(GL_RENDERBUFFER, tex);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGBA, 64, 64);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_RENDERBUFFER, tex);
    }

    glGenFramebuffers(1, &ss_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, ss_fbo);
    glGenRenderbuffers(1, &ss_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, ss_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, 64, 64);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, ss_rb);
}
