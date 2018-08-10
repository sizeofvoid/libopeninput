
.. _contributing:

==============================================================================
Contributing to libinput
==============================================================================

Contributions to libinput are always welcome. Please see the steps below for
details on how to create merge requests, correct git formatting and other
topics:

.. contents::
    :local:

Questions regarding this process can be asked on ``#wayland-devel`` on
freenode or on the `wayland-devel@lists.freedesktop.org
<https://lists.freedesktop.org/mailman/listinfo/wayland-devel>`_ mailing
list.

------------------------------------------------------------------------------
Submitting Code
------------------------------------------------------------------------------

Any patches should be sent via a Merge Request (see the `GitLab docs
<https://docs.gitlab.com/ce/gitlab-basics/add-merge-request.htm>`_)
in the `libinput GitLab instance hosted by freedesktop.org
<https://gitlab.freedesktop.org/libinput/libinput>`_.

To submit a merge request, you need to

- `Register an account <https://gitlab.freedesktop.org/users/sign_in>`_ in
  the freedesktop.org GitLab instance.
- `Fork libinput <https://gitlab.freedesktop.org/libinput/libinput/forks/new>`_
  into your username's namespace
- Get libinput's main repository: ::

    git clone https://gitlab.freedesktop.org/libinput/libinput.git

- Add the forked git repository to your remotes (replace ``USERNAME``
  with your username): ::

    cd /path/to/libinput.git
    git remote add gitlab git@gitlab.freedesktop.org:USERNAME/libinput.git
    git fetch gitlab

- Push your changes to your fork: ::

    git push gitlab BRANCHNAME

- Submit a merge request. The URL for a merge request is: ::

    https://gitlab.freedesktop.org/USERNAME/libinput/merge_requests

  Select your branch name to merge and ``libinput/libinput`` ``master`` as target branch.

------------------------------------------------------------------------------
Commit History
------------------------------------------------------------------------------

libinput strives to have a
`linear, 'recipe' style history <http://www.bitsnbites.eu/git-history-work-log-vs-recipe/>`_
This means that every commit should be small, digestible, stand-alone, and
functional. Rather than a purely chronological commit history like this: ::

	doc: final docs for view transforms
	fix tests when disabled, redo broken doc formatting
	better transformed-view iteration (thanks Hannah!)
	try to catch more cases in tests
	tests: add new spline test
	fix compilation on splines
	doc: notes on reticulating splines
	compositor: add spline reticulation for view transforms

We aim to have a clean history which only reflects the final state, broken up
into functional groupings: ::

	compositor: add spline reticulation for view transforms
	compositor: new iterator for view transforms
	tests: add view-transform correctness tests
	doc: fix Doxygen formatting for view transforms

This ensures that the final patch series only contains the final state,
without the changes and missteps taken along the development process.

The first line of a commit message should contain a prefix indicating
what part is affected by the patch followed by one sentence that
describes the change. For example: ::

	touchpad: add software button behavior
	fallback: disable button debouncing on device foo

If in doubt what prefix to use, look at other commits that change the
same file(s) as the patch being sent.

------------------------------------------------------------------------------
Commit Messages
------------------------------------------------------------------------------

Read `on commit messages <http://who-t.blogspot.de/2009/12/on-commit-messages.html>`_
as a general guideline on what commit messages should contain.

Commit messages **should** contain a **Signed-off-by** line with your name
and email address. If you're not the patch's original author, you should
also gather S-o-b's by them (and/or whomever gave the patch to you.) The
significance of this is that it certifies that you created the patch, that
it was created under an appropriate open source license, or provided to you
under those terms. This lets us indicate a chain of responsibility for the
copyright status of the code.

We won't reject patches that lack S-o-b, but it is strongly recommended.

When you re-send patches, revised or not, it would be very good to document the
changes compared to the previous revision in the commit message and/or the
merge request. If you have already received Reviewed-by or Acked-by tags, you
should evaluate whether they still apply and include them in the respective
commit messages. Otherwise the tags may be lost, reviewers miss the credit they
deserve, and the patches may cause redundant review effort.

------------------------------------------------------------------------------
Coding Style
------------------------------------------------------------------------------

Please see the `CODING_STYLE.md
<https://gitlab.freedesktop.org/libinput/libinput/blob/master/CODING_STYLE.md>`_
document in the source tree.

------------------------------------------------------------------------------
Tracking patches and follow-ups
------------------------------------------------------------------------------

Once submitted to GitLab, your patches will be reviewed by the libinput
development team on GitLab. Review may be entirely positive and result in your
code landing instantly, in which case, great! You're done. However, we may ask
you to make some revisions: fixing some bugs we've noticed, working to a
slightly different design, or adding documentation and tests.

If you do get asked to revise the patches, please bear in mind the notes above.
You should use ``git rebase -i`` to make revisions, so that your patches
follow the clear linear split documented above. Following that split makes
it easier for reviewers to understand your work, and to verify that the code
you're submitting is correct.

A common request is to split single large patch into multiple patches. This can
happen, for example, if when adding a new feature you notice a bug in
libinput's core which you need to fix to progress. Separating these changes
into separate commits will allow us to verify and land the bugfix quickly,
pushing part of your work for the good of everyone, whilst revision and
discussion continues on the larger feature part. It also allows us to direct
you towards reviewers who best understand the different areas you are
working on.

When you have made any requested changes, please rebase the commits, verify
that they still individually look good, then force-push your new branch to
GitLab. This will update the merge request and notify everyone subscribed to
your merge request, so they can review it again.

There are also many GitLab CLI clients, if you prefer to avoid the web
interface. It may be difficult to follow review comments without using the
web interface though, so we do recommend using this to go through the review
process, even if you use other clients to track the list of available
patches.

------------------------------------------------------------------------------
Code of Conduct
------------------------------------------------------------------------------

As a freedesktop.org project, libinput follows the `freedesktop.org
Contributor Covenant <https://www.freedesktop.org/wiki/CodeOfConduct>`_.

Please conduct yourself in a respectful and civilised manner when
interacting with community members on mailing lists, IRC, or bug trackers.
The community represents the project as a whole, and abusive or bullying
behaviour is not tolerated by the project.
